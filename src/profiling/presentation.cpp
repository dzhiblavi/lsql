#include "profiling/presentation.h"
#include "profiling/Counters.h"

#include "util/StrBuilder.h"
#include "util/instrument/duration.h"

#include <unordered_set>

namespace lsql::prof {

namespace {

std::string join(const std::vector<std::string>& strs, std::string_view sep) {
    if (strs.empty()) {
        return "";
    }

    std::stringstream ss;
    for (size_t i = 0; i < strs.size() - 1; ++i) {
        ss << strs[i] << sep;
    }
    ss << strs.back();
    return ss.str();
}

util::StrBuilder formatCountersList(std::span<const int64_t> counters) {
    util::StrBuilder s;
    for (CounterId id = 0; id < counters.size(); ++id) {
        if (counters[id] == 0) {
            continue;
        }

        s.item("{}: {}", CounterRegistry::instance().name(id), counters[id]);
    }
    return s;
}

util::StrBuilder metricsBaseReport(const ScopeMetricsBase& m) {
    auto b =
        util::StrBuilder()
            .line("count: {}", m.count)
            .line(
                "self: total={} avg={}",
                instr::prettyDuration(m.self_dur),
                m.count == 0 ? "0" : instr::prettyDuration(m.self_dur / m.count))
            .line(
                "total: total={} avg={}",
                instr::prettyDuration(m.total_dur),
                m.count == 0 ? "0" : instr::prettyDuration(m.total_dur / m.count));

    auto c = formatCountersList(m.counters.view());
    if (!c.empty()) {
        b.child(util::StrBuilder("counters").block(c));
    }

    return b;
}

util::StrBuilder metricsReport(const ScopeMetricsBase& m) {
    return metricsBaseReport(m).block(m.report());
}

util::StrBuilder metricsShortReport(const ScopeMetricsBase& m) {
    return metricsBaseReport(m).block(m.shortReport());
}

void appendFoldedStacks(
    const ScopeNodeSnapshot& node, std::vector<std::string>& stack, std::vector<std::string>& out) {
    if (node.metrics->empty()) {
        return;
    }

    stack.push_back(node.name);

    auto self = node.metrics->self_dur;
    if (self.count() > 0) {
        out.push_back(
            std::format(
                "{} {}",
                join(stack, ";"),
                std::chrono::duration_cast<std::chrono::nanoseconds>(self).count()));
    }

    for (auto* child : node.children) {
        appendFoldedStacks(*child, stack, out);  // expanded mode: no visited set
    }

    stack.pop_back();
}

std::string escapeDot(std::string_view s) {
    std::string out;
    out.reserve(s.size());

    for (char c : s) {
        switch (c) {
            case '\\':
                out += "\\\\";
                break;

            case '"':
                out += "\\\"";
                break;

            case '\n':
                out += "\\l";  // left-align
                break;

            case '\r':
                break;

            case '\t':
                out += "    ";
                break;

            default:
                out += c;
                break;
        }
    }

    return out;
}

struct DotStyle {
    std::string fill;
    std::string font = "black";
};

DotStyle styleFor(int64_t self, int64_t p50, int64_t p75, int64_t p90, int64_t p95) {
    if (self >= p95)
        return {.fill = "#b2182b", .font = "white"};
    if (self >= p90)
        return {.fill = "#ef8a62"};
    if (self >= p75)
        return {.fill = "#fddbc7"};
    if (self >= p50)
        return {.fill = "#f7f7f7"};
    return {.fill = "#eeeeee", .font = "#777777"};
}

double edgeWeight(const ScopeNodeSnapshot& child, int64_t max_total_ns) {
    if (child.metrics->empty() || max_total_ns == 0) {
        return 1.0;
    }

    auto x = double(child.metrics->total_dur.count()) / double(max_total_ns);
    return 1.0 + 5.0 * x;
}

void formatDotSnapshot(util::StrBuilder& b, const Snapshot& p, size_t phase_index) {
    std::string prefix = std::format("p{}_", phase_index);

    std::unordered_map<const ScopeNodeSnapshot*, size_t> ids;
    size_t next = 0;

    std::vector<int64_t> self_values;
    int64_t max_total_ns = 0;

    for (auto&& [_, node] : p.nodes) {
        if (node.metrics->empty()) {
            continue;
        }

        ids[&node] = next++;
        self_values.push_back(node.metrics->self_dur.count());
        max_total_ns = std::max(max_total_ns, (int64_t)node.metrics->total_dur.count());
    }

    std::ranges::sort(self_values);

    auto percentile = [&](double q) -> int64_t {
        if (self_values.empty()) {
            return 0;
        }

        auto idx = static_cast<size_t>(q * (self_values.size() - 1));  // NOLINT
        return self_values[idx];
    };

    auto p50 = percentile(0.50);
    auto p75 = percentile(0.75);
    auto p90 = percentile(0.90);
    auto p95 = percentile(0.95);

    b.line("  subgraph cluster_phase_{} {{", phase_index);
    b.line("    label=\"phase {}\";", phase_index);
    b.line("    color=\"#bbbbbb\";");
    b.line("    style=\"rounded\";");
    b.line("    {}anchor [style=invis, width=0, height=0, label=\"\"];", prefix);

    for (auto&& [_, node] : p.nodes) {
        if (node.metrics->empty()) {
            continue;
        }

        auto id = ids[&node];
        auto node_id = std::format("{}n{}", prefix, id);
        auto self_ns = node.metrics->self_dur.count();
        auto style = styleFor(self_ns, p50, p75, p90, p95);
        auto label = std::format("{}\n{}", node.name, metricsShortReport(*node.metrics).render());

        b.line(
            "    {} [label=\"{}\", fillcolor=\"{}\", fontcolor=\"{}\"];",
            node_id,
            escapeDot(label),
            style.fill,
            style.font);
    }

    for (auto&& [_, node] : p.nodes) {
        if (node.metrics->empty()) {
            continue;
        }
        auto from = std::format("{}n{}", prefix, ids[&node]);

        for (auto* child : node.children) {
            if (child->metrics->empty()) {
                continue;
            }

            auto to = std::format("{}n{}", prefix, ids[child]);
            auto width = edgeWeight(*child, max_total_ns);
            b.line("    {} -> {} [penwidth={}];", from, to, width);
        }
    }

    b.line("  }");
}

}  // namespace

std::string formatProfile(const Snapshot& p) {
    using util::StrBuilder;

    std::unordered_map<const ScopeNodeSnapshot*, bool> visible;
    std::unordered_set<const ScopeNodeSnapshot*> visibility_stack;

    auto hasVisibleMetrics = [&]<typename S>(this S& self, const ScopeNodeSnapshot& node) -> bool {
        if (auto it = visible.find(&node); it != visible.end()) {
            return it->second;
        }

        if (!visibility_stack.insert(&node).second) {
            return false;
        }

        bool result = !node.metrics->empty();
        for (auto* child : node.children) {
            result = result || self(*child);
        }

        visibility_stack.erase(&node);
        visible[&node] = result;
        return result;
    };

    auto formatNode = [&]<typename S>(this S& self, const ScopeNodeSnapshot& node, auto& visited) {
        if (!hasVisibleMetrics(node)) {
            return StrBuilder();
        }

        visited.insert(&node);
        if (node.metrics->empty()) {
            auto b = StrBuilder("{} - no metrics", node.name);
            auto cb = StrBuilder();
            for (auto* child : node.children) {
                if (!hasVisibleMetrics(*child)) {
                    continue;
                }

                if (visited.contains(child)) {
                    cb.item(StrBuilder("(visited) {}", child->name));
                } else {
                    cb.item(self(*child, visited));
                }
            }

            if (!cb.empty()) {
                b.child(StrBuilder("children").block(cb));
            }
            return b;
        }

        auto b = StrBuilder(node.name);
        b.child(StrBuilder("metrics").child(metricsReport(*node.metrics)));

        if (node.parents.size() > 1) {
            auto pb = StrBuilder("parents");
            for (auto* parent : node.parents) {
                if (!hasVisibleMetrics(*parent)) {
                    continue;
                }
                pb.item(StrBuilder("(see) {}", parent->name));
            }
            if (!pb.empty()) {
                b.child(pb);
            }
        }

        if (!node.children.empty()) {
            auto cb = StrBuilder();
            for (auto* child : node.children) {
                if (!hasVisibleMetrics(*child)) {
                    continue;
                }

                if (visited.contains(child)) {
                    cb.item(StrBuilder("(visited) {}", child->name));
                } else {
                    cb.item(self(*child, visited));
                }
            }
            if (!cb.empty()) {
                b.child(StrBuilder("children").block(cb));
            }
        }

        return b;
    };

    std::unordered_set<const ScopeNodeSnapshot*> visited;
    auto b = StrBuilder("Profiler report");

    for (auto&& [_, node] : p.nodes) {
        if (!node.is_root) {
            continue;
        }

        if (auto nb = formatNode(node, visited); !nb.empty()) {
            b.item(nb);
        }
    }
    return b.render();
}

std::string formatFoldedStacks(const Snapshot& p) {
    std::vector<std::string> lines;
    std::vector<std::string> stack;

    for (auto&& [_, node] : p.nodes) {
        if (node.is_root) {
            appendFoldedStacks(node, stack, lines);
        }
    }

    std::ranges::sort(lines);
    return join(lines, "\n");
}

std::string formatDot(std::span<const Snapshot> phases) {
    util::StrBuilder b;

    b.line("digraph profile {");
    b.line("  rankdir=LR;");
    b.line("  compound=true;");
    b.line("  node [shape=box, style=\"rounded,filled\", fontname=\"Menlo\"];");
    b.line("  edge [color=\"#888888\"];");

    for (size_t i = 0; i < phases.size(); ++i) {
        formatDotSnapshot(b, phases[i], i);
    }

    // enforce phase ordering
    if (phases.size() > 1) {
        b.line("");

        for (size_t i = 0; i + 1 < phases.size(); ++i) {
            b.line("  p{}_anchor -> p{}_anchor [style=invis, weight=1000];", i, i + 1);
        }
    }

    b.line("}");
    return b.render();
}

}  // namespace lsql::prof
