#include "ir/tools.h"

namespace lsql::ir {

FieldSet outputFieldsOf(const std::vector<ir::Projector>& ps) {
    auto fields = FieldSet::emptySet();
    for (auto&& p : ps) {
        fields.add(p.alias_field_id);
    }
    return fields;
}

FieldSet outputFieldsOf(const std::vector<ir::Aggregate>& ps) {
    auto fields = FieldSet::emptySet();
    for (auto&& p : ps) {
        fields.add(p.output_field_id);
    }
    return fields;
}

}  // namespace lsql::ir
