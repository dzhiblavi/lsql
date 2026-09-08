#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

#include <simdjson/ondemand.h>
#include <simdjson/padded_string-inl.h>

namespace lsql::func {

struct JSONValueExecutor {
    explicit JSONValueExecutor(std::string path) : path(std::move(path)) {}

    Value execute(Value value) const {
        if (value == vnull) {
            return vnull;
        }

        simdjson::padded_string json(value.get<std::string_view>());
        static thread_local simdjson::ondemand::parser parser;

        auto document = parser.iterate(json);
        auto selected = document.at_pointer(path);
        simdjson::ondemand::json_type type;
        if (selected.type().get(type)) {
            return vnull;
        }

        switch (type) {
            case simdjson::ondemand::json_type::string: {
                std::string_view result;
                if (selected.get_string().get(result)) {
                    return vnull;
                }
                return std::string(result);
            }

            case simdjson::ondemand::json_type::number: {
                std::string_view result;
                if (selected.raw_json_token().get(result)) {
                    return vnull;
                }
                const auto end = result.find_last_not_of(" \t\r\n");
                return std::string(result.substr(0, end + 1));
            }

            case simdjson::ondemand::json_type::boolean: {
                bool result = false;
                if (selected.get_bool().get(result)) {
                    return vnull;
                }
                return std::string(result ? "true" : "false");
            }

            case simdjson::ondemand::json_type::array:
            case simdjson::ondemand::json_type::object: {
                std::string_view result;
                if (selected.raw_json().get(result)) {
                    return vnull;
                }
                return std::string(result);
            }

            case simdjson::ondemand::json_type::null:
                return vnull;
        }
    }

    std::string path;
};

static_assert(UnaryExecutor<JSONValueExecutor>);

inline JSONValueExecutor build(const JSONValue& s) {
    return JSONValueExecutor(s.path);
}

}  // namespace lsql::func
