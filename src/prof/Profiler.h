#pragma once

#include "prof/Scope.h"

#include "util/StrBuilder.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace lsql::prof {

class Profiler {
    using StrBuilder = util::StrBuilder;

    struct ScopeNode {
        bool is_root;
        std::string name;
        std::unique_ptr<MetricsBase> metrics;
        std::vector<ScopeNode*> children;
        std::vector<ScopeNode*> parents;
    };

 public:
    template <Metrics M, typename... Args>
    ScopeHandle<M> newScope(std::string name, Args&&... args) {
        auto metrics = std::make_unique<M>(std::forward<Args>(args)...);
        M* metrics_ptr = metrics.get();
        auto node = ScopeNode{
            .is_root = true,
            .name = std::move(name),
            .metrics = std::move(metrics),
            .children = {},
            .parents = {},
        };

        nodes_.emplace(metrics_ptr, std::move(node));
        return ScopeHandle<M>(metrics_ptr);
    }

    // Idempotent
    void addEdge(MetricsBase* parent, MetricsBase* child) {
        if (parent == nullptr || child == nullptr) {
            return;
        }

        auto pit = nodes_.find(parent);
        verify(pit != nodes_.end());
        auto cit = nodes_.find(child);
        verify(cit != nodes_.end());

        auto& child_node = cit->second;
        auto& parent_node = pit->second;

        if (std::ranges::find(parent_node.children, &child_node) != parent_node.children.end()) {
            return;
        }

        parent_node.children.push_back(&child_node);
        child_node.parents.push_back(&parent_node);
        child_node.is_root = false;
    }

    std::string format() const {
        std::unordered_set<const ScopeNode*> visited;
        auto b = StrBuilder("Profiler report");

        for (auto&& [_, node] : nodes_) {
            if (!node.is_root) {
                continue;
            }

            if (auto nb = formatNode(node, visited); !nb.empty()) {
                b.item(nb);
            }
        }
        return b.render();
    }

    void reset() {
        for (auto&& [_, node] : nodes_) {
            node.metrics->reset();
        }
    }

 private:
    static StrBuilder formatNode(const ScopeNode& node, auto& visited) {
        visited.insert(&node);
        if (node.metrics->empty()) {
            return StrBuilder("{} - no metrics", node.name);
        }

        auto b = StrBuilder(node.name);
        b.child(StrBuilder("metrics").child(node.metrics->report()));

        if (node.parents.size() > 1) {
            auto pb = StrBuilder("parents");
            for (auto* parent : node.parents) {
                pb.item(StrBuilder("(see) {}", parent->name));
            }
            b.child(pb);
        }

        if (!node.children.empty()) {
            auto cb = StrBuilder();
            for (auto* child : node.children) {
                if (visited.contains(child)) {
                    cb.item(StrBuilder("(visited) {}", child->name));
                } else {
                    cb.item(formatNode(*child, visited));
                }
            }
            b.child(StrBuilder("children").block(cb));
        }

        return b;
    }

    std::unordered_map<MetricsBase*, ScopeNode> nodes_;
};

}  // namespace lsql::prof
