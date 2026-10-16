#pragma once

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

    template <typename... Args>
    void visit(std::variant<Args...>& value) {
        std::visit([&](auto& x) { doVisit(x); }, value);
    }

    template <typename... Args>
    void visit(const std::variant<Args...>& value) {
        std::visit([&](const auto& x) { doVisit(x); }, value);
    }

    template <typename T>
    void visit(std::vector<T>& value) {
        for (auto& x : value) {
            doVisit(x);
        }
    }

    template <typename T>
    void visit(const std::vector<T>& value) {
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
    void visit(const std::optional<T>& value) {
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
    void visit(const std::shared_ptr<T>& value) {
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
    void visit(const std::unique_ptr<T>& value) {
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
