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

struct Snapshot {
    std::unordered_map<const ScopeMetricsBase*, ScopeNodeSnapshot> nodes;
};

class Profiler {
 public:
    template <CScopeMetrics M, typename... Args>
    ScopeHandle<M> newScope(std::string name, Args&&... args);

    // Idempotent
    void addEdge(ScopeHandleBase parent, ScopeHandleBase child);
    void reset();
    Snapshot snapshot() const;

 private:
    ScopeNodeSnapshot* snapshot(const ScopeNode& n, auto& nodes, auto& visited) const;

    std::unordered_map<ScopeMetricsBase*, ScopeNode> nodes_;
};

template <CScopeMetrics M, typename... Args>
ScopeHandle<M> Profiler::newScope(std::string name, Args&&... args) {
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

}  // namespace lsql::prof
