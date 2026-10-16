#pragma once

#include "ir/Scalars.h"
#include "ir/reflect.h"

namespace lsql::ir {

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
