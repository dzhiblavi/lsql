#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace lsql::prof {

using CounterId = uint32_t;

class CounterRegistry {
 public:
    CounterRegistry() = default;

    CounterId bind(std::string name);
    std::string_view name(CounterId id);

    static CounterRegistry& instance();

 private:
    std::unordered_map<CounterId, std::string> names_;
    std::unordered_map<std::string, CounterId> ids_;
    CounterId next_id_ = 0;
};

}  // namespace lsql::prof
