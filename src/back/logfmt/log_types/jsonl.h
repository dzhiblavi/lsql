#pragma once

#include "back/logfmt/log_types/impl.h"
#include "core/time_formats.h"
#include "core/value/null_t.h"

#include <simdjson/ondemand.h>
#include <simdjson/padded_string-inl.h>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace lsql::back::logfmt {

template <>
struct LogTypeImpl<LogType::JSONL> {
    static constexpr std::optional<TimeFormat> time_format = std::nullopt;

    template <typename F>
    static bool parseKeyValue(std::string_view line, F&& callback) {
        simdjson::padded_string json(line);
        static thread_local simdjson::ondemand::parser parser;
        simdjson::ondemand::document document;
        if (parser.iterate(json).get(document)) {
            return false;
        }

        simdjson::ondemand::object object;
        if (document.get_object().get(object)) {
            return false;
        }

        for (auto entry : object) {
            simdjson::ondemand::field field;
            if (std::move(entry).get(field)) {
                return false;
            }
            std::string_view key;
            if (field.unescaped_key(false).get(key)) {
                return false;
            }

            auto& value = field.value();
            simdjson::ondemand::json_type type;
            if (value.type().get(type)) {
                return false;
            }

            if (type == simdjson::ondemand::json_type::string) {
                std::string_view decoded;
                if (value.get_string().get(decoded)) {
                    return false;
                }
                callback(key, std::string(decoded));
            } else {
                std::string_view raw;
                if (value.raw_json().get(raw)) {
                    return false;
                }
                raw = raw.substr(0, raw.find_last_not_of(" \t\r\n") + 1);
                if (raw == "null") {
                    callback(key, null);
                } else {
                    callback(
                        key,
                        line.substr(static_cast<size_t>(raw.data() - json.data()), raw.size()));
                }
            }
        }

        return document.at_end();
    }

    static bool detectLogType(std::string_view line) {
        const auto begin = line.find_first_not_of(" \t\r\n");
        return begin != std::string_view::npos && line[begin] == '{' &&
               parseKeyValue(line, [](auto&&...) {});
    }
};

}  // namespace lsql::back::logfmt
