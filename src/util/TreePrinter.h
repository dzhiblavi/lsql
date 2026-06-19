#pragma once

#include "util/StrBuilder.h"
#include "util/concepts.h"
#include "util/string.h"

#include <rfl.hpp>

#include <magic_enum/magic_enum.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace lsql::util {

template <typename Self>
struct TreePrinter {
 public:
    StrBuilder print(const std::string& s) { return s; }
    StrBuilder print(char s) { return std::string(1, s); }
    StrBuilder print(float s) { return std::to_string(s); }
    StrBuilder print(bool s) { return std::to_string(s); }
    StrBuilder print(int s) { return std::to_string(s); }
    StrBuilder print(int64_t s) { return std::to_string(s); }
    StrBuilder print(size_t s) { return std::to_string(s); }

    template <util::InstanceOf<std::variant> T>
    StrBuilder print(const T& t) {
        return std::visit([&](const auto& u) { return call(u); }, t);
    }

    template <typename T>
    StrBuilder print(const std::vector<T>& vs) {
        StrBuilder list;
        for (auto&& v : vs) {
            list.item(call(v));
        }
        return list;
    }

    template <typename T>
    StrBuilder print(const std::unique_ptr<T>& b) {
        return call(*b);
    }

    template <typename T>
    StrBuilder print(const std::shared_ptr<T>& b) {
        return call(*b);
    }

    template <typename T>
    StrBuilder print(const std::optional<T>& v) {
        return v.has_value() ? call(*v) : StrBuilder();
    }

    template <typename E>
    requires(std::is_enum_v<E>)
    StrBuilder print(E e) {
        return StrBuilder(std::string(magic_enum::enum_name(e)));
    }

    template <typename T>
    requires(!std::is_enum_v<T> && !util::InstanceOf<T, std::variant>)
    StrBuilder print(const T& val) {
        StrBuilder b;
        b.line("type: {}", stripNamespace(rfl::type_name_t<T>().name()));

        rfl::to_view(val).apply([&](const auto& f) {
            auto child = call(*f.value());
            if (child.empty()) {
                return;
            }

            if (child.lines() == 1) {
                b.block(StrBuilder("{}: {}", f.name(), child.render()));
            } else {
                b.block(StrBuilder("{}:", f.name()).child(child));
            }
        });
        return b;
    }

 private:
    template <typename T>
    StrBuilder call(const T& t) {
        return static_cast<Self*>(this)->print(t);
    }
};

}  // namespace lsql::util
