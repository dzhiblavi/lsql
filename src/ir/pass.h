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

    std::vector<ir::Scalar> pass(std::vector<ir::Scalar> ag) {
        std::vector<ir::Scalar> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(std::move(a)));
        }
        return r;
    }

    std::vector<ir::Aggregate> pass(std::vector<ir::Aggregate> ag) {
        std::vector<ir::Aggregate> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(std::move(a)));
        }
        return r;
    }

    std::vector<ir::Projector> pass(std::vector<ir::Projector> ag) {
        std::vector<ir::Projector> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back({
                .alias_field_id = a.alias_field_id,
                .expr = box(pass(std::move(*a.expr))),
            });
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

    template <std::same_as<void> U = R>
    void pass(const std::vector<ir::Scalar>& ag) {
        for (auto&& a : ag) {
            pass(a);
        }
    }

    template <typename U = R>
    requires(!std::same_as<U, void>)
    std::vector<R> pass(const std::vector<ir::Scalar>& ag) {
        std::vector<R> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(a));
        }
        return r;
    }

    template <std::same_as<void> U = R>
    void pass(const std::vector<ir::Aggregate>& ag) {
        for (auto&& a : ag) {
            pass(a);
        }
    }

    template <typename U = R>
    requires(!std::same_as<U, void>)
    std::vector<R> pass(const std::vector<ir::Aggregate>& ag) {
        std::vector<R> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(a));
        }
        return r;
    }

    template <std::same_as<void> U = R>
    void pass(const std::vector<ir::Projector>& ag) {
        for (auto&& a : ag) {
            pass(a);
        }
    }

    template <typename U = R>
    requires(!std::same_as<U, void>)
    std::vector<R> pass(const std::vector<ir::Projector>& ag) {
        std::vector<R> r;
        r.reserve(ag.size());
        for (auto&& a : ag) {
            r.push_back(pass(*a.expr));
        }
        return r;
    }
};

}  // namespace lsql::ir
