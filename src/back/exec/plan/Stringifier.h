#pragma once

#include "back/exec/plan/plan.h"

#include "core/TreePrinter.h"
#include "util/StrBuilder.h"

namespace lsql::back::exec::plan {

struct Stringifier : TreePrinter<Stringifier> {
    using TreePrinter<Stringifier>::print;

    std::string print(const Plan& p) {
        binding = p.field_binding;

        auto b = StrBuilder("Program");
        for (auto&& st : p.top_operations) {
            b.item(print(st));
        }
        return b.render();
    }

    StrBuilder print(const exec::Aggregate&) { return StrBuilder("<compiled>"); }
    StrBuilder print(const exec::Scalar&) { return StrBuilder("<compiled>"); }

    StrBuilder print(const Operation& op) {
        if (printed.contains(&op)) {
            return StrBuilder("(see id={})", op.id);
        }

        printed.insert(&op);
        return TreePrinter<Stringifier>::print(op);
    }

    StrBuilder print(const std::map<int, FieldSet>& required_fields) {
        StrBuilder b;
        for (auto&& [phase, fields] : required_fields) {
            StrBuilder p;
            p.block(std::format("phase: {}", phase));
            p.block(std::format("fields: {}", to_string(fields, *binding)));
            b.item(p);
        }
        return b;
    }

    std::unordered_set<const Operation*> printed;
};

}  // namespace lsql::back::exec::plan
