#include "cli/settings.h"

#include "core/exceptions.h"

#include "config/optimizer_settings.h"
#include "config/storage_settings.h"

#include "util/string_cast.h"

namespace lsql {

namespace {

void set(std::string_view group, std::string_view key, int64_t value) {
    if (group == "Optimizer") {
        auto maybe_setting = magic_enum::enum_cast<config::OptimizerSetting>(key);
        require(maybe_setting.has_value(), "invalid setting for {} group: {}", group, key);
        config::optimizer().set(*maybe_setting, value);
        return;
    }

    if (group == "Storage") {
        auto maybe_setting = magic_enum::enum_cast<config::StorageSetting>(key);
        require(maybe_setting.has_value(), "invalid setting for {} group: {}", group, key);
        config::storage().set(*maybe_setting, value);
        return;
    }

    throwError("invalid group: {}", group);
}

}  // namespace

void parseSettings(std::span<const std::string> options) {
    for (auto&& option : options) {
        auto assignment = std::string_view(option);
        auto pos = assignment.find('=');
        require(pos != std::string::npos, "invalid option: -o {}", assignment);

        auto value_str = assignment.substr(pos + 1);
        auto value = util::parseInt64Strict(value_str);
        require(value.has_value(), "invalid value for int64_t: {}", value_str);

        auto key = assignment.substr(0, pos);
        pos = key.find('.');
        require(pos != std::string::npos, "invalid option key: {}", key);

        auto group = key.substr(0, pos);
        key = key.substr(pos + 1);

        set(group, key, *value);
    }
}

void dumpSettings(std::ostream& os, std::string_view prefix) {
    for (auto value : magic_enum::enum_values<config::OptimizerSetting>()) {
        os << std::format(
                  "{}Optimizer.{}={}",
                  prefix,
                  magic_enum::enum_name(value),
                  config::optimizer().get(value))
           << '\n';
    }
    for (auto value : magic_enum::enum_values<config::StorageSetting>()) {
        os << std::format(
                  "{}Storage.{}={}",
                  prefix,
                  magic_enum::enum_name(value),
                  config::storage().get(value))
           << '\n';
    }
}

}  // namespace lsql
