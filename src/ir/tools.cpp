#include "ir/tools.h"

namespace lsql::ir {

Schema schemaFor(const std::vector<ir::Projector>& ps) {
    Schema s;
    for (auto&& p : ps) {
        s.append(p.alias_field_id);
    }
    return s;
}

Schema schemaFor(const std::vector<ir::Aggregate>& ps) {
    Schema s;
    for (auto&& p : ps) {
        s.append(p.output_field_id);
    }
    return s;
}

}  // namespace lsql::ir
