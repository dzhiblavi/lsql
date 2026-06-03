#pragma once

#include "profiling/Scope.h"

#include <algorithm>
#include <vector>

namespace lsql::prof {

struct ScopeNode {
    bool is_root;
    std::string name;
    std::unique_ptr<ScopeMetricsBase> metrics;
    std::vector<ScopeNode*> children;
    std::vector<ScopeNode*> parents;
};

struct ScopeNodeSnapshot {
    bool is_root;
    std::string name;
    std::unique_ptr<const ScopeMetricsBase> metrics;
    std::vector<const ScopeNodeSnapshot*> children;
    std::vector<const ScopeNodeSnapshot*> parents;
};

class Profiler {
 public:
    using Snapshot = std::unordered_map<const ScopeMetricsBase*, ScopeNodeSnapshot>;

    template <CScopeMetrics M, typename... Args>
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
    void addEdge(ScopeMetricsBase* parent, ScopeMetricsBase* child) {
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

    void reset() {
        for (auto&& [_, node] : nodes_) {
            node.metrics->reset();
        }
    }

    std::unordered_map<const ScopeMetricsBase*, ScopeNodeSnapshot> snapshot() const {
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

 private:
    ScopeNodeSnapshot* snapshot(const ScopeNode& n, auto& nodes, auto& visited) const {
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

    std::unordered_map<ScopeMetricsBase*, ScopeNode> nodes_;
};

}  // namespace lsql::prof
