#pragma once

#include "iface/sql/ast/Literal.h"
#include "iface/sql/bound/Expressions.h"
#include "iface/sql/bound/Relations.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/require.h"

#include <format>

namespace lsql::iface::sql::bind {

inline std::string removeQuotes(const std::string& s) {
    require(s.size() >= 2, "string literal is too small");
    return s.substr(1, s.size() - 2);
}

inline Value parseLiteral(ast::Literal literal) {
    switch (literal.type) {
        case ValueType::Null:
            return null;

        case ValueType::String:
            return removeQuotes(literal.value_str);

        case ValueType::Integer:
            return int64_t(std::stoll(literal.value_str));

        case ValueType::Floating:
            return float(std::strtof(literal.value_str.data(), nullptr));

        case ValueType::Boolean:
            require(
                literal.value_str == "true" || literal.value_str == "false",
                "invalid boolean literal");
            return literal.value_str == "true";
    }
}

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

}  // namespace lsql::iface::sql::bind
