#include "profiling/Profiler.h"

namespace lsql::prof {

void Profiler::addEdge(ScopeMetricsBase* parent, ScopeMetricsBase* child) {
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

void Profiler::reset() {
    for (auto&& [_, node] : nodes_) {
        node.metrics->reset();
    }
}

std::unordered_map<const ScopeMetricsBase*, ScopeNodeSnapshot> Profiler::snapshot() const {
    // metrics -> snapshot node pointer (resides in nodes)
    std::unordered_map<const ScopeMetricsBase*, ScopeNodeSnapshot*> visited;
    // snapsnot metrics -> snapshot node
    std::unordered_map<const ScopeMetricsBase*, ScopeNodeSnapshot> nodes;

    for (auto&& [metrics, node] : nodes_) {
        if (node.is_root) {
            snapshot(node, nodes, visited);
        }
    }

    // fill parents
    for (auto&& [metrics, node] : nodes_) {
        auto* snapshot = visited.at(metrics);

        for (auto* parent : node.parents) {
            auto* parent_snapshot = visited.at(parent->metrics.get());
            snapshot->parents.push_back(parent_snapshot);
        }
    }

    return nodes;
}

ScopeNodeSnapshot* Profiler::snapshot(const ScopeNode& n, auto& nodes, auto& visited) const {
    if (auto it = visited.find(n.metrics.get()); it != visited.end()) {
        return it->second;
    }

    auto metrics = n.metrics->clone();

    auto [it, _] = nodes.emplace(
        metrics.get(),
        ScopeNodeSnapshot{
            .is_root = n.is_root,
            .name = n.name,
            .metrics = std::move(metrics),
            .children = {},
            .parents = {},
        });

    auto* ptr = &it->second;
    visited[n.metrics.get()] = ptr;

    ptr->children.reserve(n.children.size());
    for (auto* child : n.children) {
        ptr->children.push_back(snapshot(*child, nodes, visited));
    }

    return ptr;
}

}  // namespace lsql::prof
