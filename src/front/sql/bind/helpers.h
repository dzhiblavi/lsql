#pragma once

#include "front/sql/bound/Expressions.h"
#include "front/sql/bound/Relations.h"

#include "core/Fields.h"

namespace lsql::front::sql::bind {

inline FieldSet requiredFieldsOf(const bound::Projector& p) {
    return util::match(
        p,
        [](const bound::StarProjector&) { return FieldSet::emptySet(); },
        [](const bound::IdentifierProjector& p) { return FieldSet::withField(p.field_id); },
        [](const bound::ExprProjector& p) { return p.expr->required_fields; });
}

inline FieldSet requiredFieldsOf(const std::vector<bound::Projector>& ps) {
    auto fields = FieldSet::emptySet();
    for (auto&& p : ps) {
        fields.merge(requiredFieldsOf(p));
    }
    return fields;
}

inline FieldSet requiredFieldsOf(const std::vector<bound::Expr>& ps) {
    auto fields = FieldSet::emptySet();
    for (auto&& p : ps) {
        fields.merge(p.required_fields);
    }
    return fields;
}

inline FieldSet outputFieldsOf(const bound::Projector& p) {
    return util::match(
        p,
        [](const bound::StarProjector&) { return FieldSet::emptySet(); },
        [](const bound::IdentifierProjector& p) { return FieldSet::withField(p.field_id); },
        [](const bound::ExprProjector& p) { return FieldSet::withField(p.alias_field_id); });
}

inline FieldSet outputFieldsOf(const std::vector<bound::Projector>& ps) {
    auto fields = FieldSet::emptySet();
    for (auto&& p : ps) {
        fields.merge(outputFieldsOf(p));
    }
    return fields;
}

inline std::unordered_map<FieldId, bound::Projector*> buildMap(std::vector<bound::Projector>& ps) {
    std::unordered_map<FieldId, bound::Projector*> map;
    for (auto&& p : ps) {
        util::matchPartial(
            p,
            [&](bound::IdentifierProjector& pp) { map.emplace(pp.field_id, &p); },
            [&](bound::ExprProjector& pp) { map.emplace(pp.alias_field_id, &p); });
    }
    return map;
}

}  // namespace lsql::front::sql::bind
