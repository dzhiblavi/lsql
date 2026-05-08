#pragma once

#include "exec/expr/Expression.h"
#include "rel/Relation.h"

#include <algorithm>
#include <vector>

namespace lsql::rel {

using SortList = std::vector<exec::ExpressionPtr>;

class SortRelation : public Relation, public std::enable_shared_from_this<SortRelation> {
 public:
    SortRelation(RelationPtr rel, bool desc, SortList sort_list)
        : rel_(rel)
        , desc_(desc)
        , sort_list_(std::move(sort_list)) {
        if (sort_list_.empty()) {
            throw std::runtime_error("ORDER BY list cannot be empty");
        }
    }

    coro::generator<const Record*> records() const override {
        std::vector<ConstRecordPtr> records;

        for (auto* record : rel_->records()) {
            records.push_back(record->clone());
        }

        if (desc_) {
            std::sort(records.begin(), records.end(), [this](auto&& l, auto&& r) {
                return key(*l) > key(*r);
            });
        } else {
            std::sort(records.begin(), records.end(), [this](auto&& l, auto&& r) {
                return key(*l) < key(*r);
            });
        }

        for (auto&& rec : records) {
            co_yield rec.get();
        }
    }

 private:
    std::vector<Value> key(const Record& record) const {
        std::vector<Value> result;
        for (auto&& col : sort_list_) {
            result.push_back(col->eval(record));
        }
        return result;
    }

    RelationPtr rel_;
    bool desc_;
    SortList sort_list_;
};

RelationPtr executeSort(SortList glist, bool desc, RelationPtr rel) {
    return std::make_shared<SortRelation>(rel, desc, std::move(glist));
}

}  // namespace lsql::rel
