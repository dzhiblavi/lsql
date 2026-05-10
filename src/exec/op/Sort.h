#pragma once

#include "exec/expr/Expression.h"
#include "exec/op/Operation.h"
#include "core/verify.h"

#include <llog/log.h>

#include <algorithm>
#include <vector>

namespace lsql::exec {

using SortList = std::vector<exec::ExpressionPtr>;

class Sort : public Operation, public std::enable_shared_from_this<Sort> {
 public:
    Sort(OperationPtr source, bool desc, SortList sort_list)
        : Operation(1, source->minPhase())
        , source_(std::move(source))
        , desc_(desc)
        , sort_list_(std::move(sort_list)) {
        if (sort_list_.empty()) {
            throw std::runtime_error("ORDER BY list cannot be empty");
        }
    }

 private:
    bool consume(int phase, const exec::Record* record) {
        if (curr_phase_ != phase) {
            curr_phase_ = phase;
            verify(records_.empty());
        }

        if (record != nullptr) {
            records_.push_back(record->clone());

            if (!active(phase)) {
                records_.clear();
                return false;
            }
            return true;
        }

        // end of stream
        llog::info("sort dataset size: {} (phase {})", records_.size(), phase);

        if (desc_) {
            std::sort(records_.begin(), records_.end(), [this](auto&& l, auto&& r) {
                return key(*l) > key(*r);
            });
        } else {
            std::sort(records_.begin(), records_.end(), [this](auto&& l, auto&& r) {
                return key(*l) < key(*r);
            });
        }

        for (auto&& record : records_) {
            if (!emit(phase, record.get())) {
                return false;
            }
        }

        records_.clear();
        return emit(phase, nullptr);
    }

    void init(int phase) override { source_->subscribe(phase, &sub_); }

    std::vector<Value> key(const exec::Record& record) const {
        std::vector<Value> result;
        result.reserve(sort_list_.size());
        for (auto&& col : sort_list_) {
            result.push_back(col->eval(record));
        }
        return result;
    }

    OperationPtr source_;
    bool desc_;
    SortList sort_list_;
    MemberSubscriber<Sort> sub_{this, &Sort::consume};

    // phase state
    int curr_phase_ = 0;
    std::vector<exec::ConstRecordPtr> records_;
};

OperationPtr sort(OperationPtr source, SortList glist, bool desc) {
    return std::make_shared<Sort>(std::move(source), desc, std::move(glist));
}

}  // namespace lsql::exec
