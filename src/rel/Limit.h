#pragma once

#include "rel/Relation.h"

namespace lsql::rel {

class LimitRelation : public Relation {
 public:
    LimitRelation(int limit, RelationPtr rel) : rel_(rel), limit_(limit) {}

    coro::generator<const Record*> records() const override {
        if (limit_ == 0) {
            co_return;
        }

        int counter = 0;

        for (auto* record : rel_->records()) {
            co_yield record;

            if (++counter >= limit_) {
                co_return;
            }
        }
    }

 private:
    RelationPtr rel_;
    int limit_;
};

RelationPtr executeLimit(int limit, RelationPtr rel) {
    return std::make_shared<LimitRelation>(limit, std::move(rel));
}

}  // namespace lsql::rel
