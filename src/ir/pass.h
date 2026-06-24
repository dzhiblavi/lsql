#pragma once

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep
#include "ir/Statement.h"
#include "ir/reflect.h"

namespace lsql::ir {

template <typename Self>
class ConsumePass {
 public:
    ir::Relation pass(ir::Relation rel) {
        return util::match(rel.node, [&](auto& r) -> ir::Relation { return pass(r, rel); });
    }

    ir::Statement pass(ir::Statement st) {
        return util::match(st, [&](auto& s) -> ir::Statement { return pass(s, st); });
    }

    ir::Scalar pass(ir::Scalar sc) {
        return util::match(sc.node, [&](auto& s) -> ir::Scalar { return pass(s, sc); });
    }

    ir::Aggregate pass(ir::Aggregate ag) {
        return util::match(ag.node, [&](auto& s) -> ir::Aggregate { return pass(s, ag); });
    }

    ir::Projector pass(ir::Projector p) {
        return {
            .alias_field_id = p.alias_field_id,
            .expr = box(pass(std::move(*p.expr))),
        };
    }

 private:
    template <typename E>
    E pass(auto& node, E& self) {
        using Reflection = Reflect<std::decay_t<decltype(node)>>;
        auto modify = [&](auto field) { node.*field = pass(std::move(node.*field)); };
        std::apply([&](auto&&... fields) { (modify(fields), ...); }, Reflection::childNodes());
        return static_cast<Self*>(this)->construct(node, self);
    }

    template <typename E>
    Box<E> pass(Box<E> x) {
        return box(pass(std::move(*x)));
    }

    template <typename T>
    std::vector<T> pass(std::vector<T> ag) {
        std::vector<T> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(std::move(a)));
        }
        return r;
    }
};

template <typename Self, typename R>
class ScalarViewPass {
 public:
    R pass(const ir::Scalar& sc) {
        return util::match(sc.node, [&](auto& s) -> R { return pass(s, sc); });
    }

 private:
    template <typename E>
    R pass(auto& node, E& self) {
        using Reflection = Reflect<std::decay_t<decltype(node)>>;
        auto do_view = [&](auto field) { return pass(node.*field); };

        if constexpr (std::is_same_v<R, void>) {
            std::apply([&](auto&&... fields) { (do_view(fields), ...); }, Reflection::childNodes());
            static_cast<Self*>(this)->view(node, self);
        } else {
            return std::apply(
                [&](auto&&... fields) {
                    return static_cast<Self*>(this)->view(node, self, do_view(fields)...);
                },
                Reflection::childNodes());
        }
    }

    template <typename E>
    R pass(const Box<E>& x) {
        return pass(*x);
    }

    template <typename T, std::same_as<void> U = R>
    void pass(const std::vector<T>& ag) {
        for (auto&& a : ag) {
            pass(a);
        }
    }

    template <typename T, typename U = R>
    requires(!std::same_as<U, void>)
    std::vector<R> pass(const std::vector<T>& ag) {
        std::vector<R> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(a));
        }
        return r;
    }
};

}  // namespace lsql::ir
