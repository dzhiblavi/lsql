#pragma once

#include "exec/expr/Expression.h"
#include "rel/Relation.h"

namespace lsql::rel {

class FilterRelation : public Relation {
 public:
    FilterRelation(std::shared_ptr<exec::Expression> condition, RelationPtr rel)
        : condition_(condition)
        , rel_(rel) {}

    coro::generator<const Record*> records() const override {
        for (auto* record : rel_->records()) {
            if (trueish(condition_->eval(*record))) {
                co_yield record;
            }
        }
    }

 private:
    std::shared_ptr<exec::Expression> condition_;
    RelationPtr rel_;
};

RelationPtr executeFilter(std::shared_ptr<exec::Expression> condition, RelationPtr rel) {
    return std::make_shared<FilterRelation>(std::move(condition), std::move(rel));
}

}  // namespace lsql::rel
