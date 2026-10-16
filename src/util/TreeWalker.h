#pragma once

#include "util/concepts.h"

#include <rfl/to_view.hpp>

#include <memory>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

namespace lsql::util {

template <typename Self>
class TreeWalker {
 public:
    // Default visitor
    template <typename T>
    void visit(T&) {}

    template <util::InstanceOf<std::variant> T>
    void visit(T& value) {
        std::visit([&](auto& x) { doVisit(x); }, value);
    }

    template <typename T>
    void visit(std::vector<T>& value) {
        for (auto& x : value) {
            doVisit(x);
        }
    }

    template <typename T>
    void visit(std::optional<T>& value) {
        if (value.has_value()) {
            doVisit(*value);
        }
    }

    template <typename T>
    void visit(std::shared_ptr<T>& value) {
        if (value) {
            doVisit(*value);
        }
    }

    template <typename T>
    void visit(std::unique_ptr<T>& value) {
        if (value) {
            doVisit(*value);
        }
    }

    template <typename T>
    requires std::is_aggregate_v<T>
    void visit(T& value) {
        rfl::to_view(value).apply([&](const auto& field) { doVisit(*field.value()); });
    }

 private:
    template <typename T>
    void doVisit(T& value) {
        static_cast<Self*>(this)->visit(value);
    }
};

}  // namespace lsql::util
