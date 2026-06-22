#include "profiling/Counters.h"

#include "util/verify.h"

namespace lsql::prof {

CounterId CounterRegistry::bind(std::string name) {
    verify(!ids_.contains(name));
    auto id = next_id_++;
    ids_.emplace(name, id);
    names_.emplace(id, std::move(name));
    return id;
}

std::string_view CounterRegistry::name(CounterId id) {
    auto it = names_.find(id);
    verify(it != names_.end());
    return it->second;
}

CounterRegistry& CounterRegistry::instance() {
    static CounterRegistry instance;
    return instance;
}

}  // namespace lsql::prof
