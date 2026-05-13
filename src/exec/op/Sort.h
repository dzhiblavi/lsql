#pragma once

#include "core/verify.h"
#include "exec/expr/Expression.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/Operation.h"
#include "util/instrument/Timer.h"

#include <llog/log.h>

#include <algorithm>
#include <vector>

namespace lsql::exec {

using SortList = std::vector<ExpressionPtr>;

class Sort : public Operation, public std::enable_shared_from_this<Sort> {
    using Key = std::vector<Value>;

 public:
    Sort(OperationPtr source, bool desc, SortList sort_list)
        : Operation(source->minPhase(), "Sort")
        , source_(std::move(source))
        , desc_(desc)
        , sort_list_(std::move(sort_list)) {
        if (sort_list_.empty()) {
            throw std::runtime_error("ORDER BY list cannot be empty");
        }
    }

 private:
    bool consume(int phase, const Record* record) {
        if (curr_phase_ != phase) {
            curr_phase_ = phase;
            verify(records_.empty());
        }

        if (record != nullptr) {
            records_.emplace_back(record->clone(), key(*record));

            if (!active(phase)) {
                records_.clear();
                return false;
            }
            return true;
        }

        // end of stream
        instr::Timer timer;

        if (desc_) {
            std::sort(records_.begin(), records_.end(), [](auto&& l, auto&& r) {
                return l.second > r.second;
            });
        } else {
            std::sort(records_.begin(), records_.end(), [](auto&& l, auto&& r) {
                return l.second < r.second;
            });
        }

        if (auto curr = prof_.current()) {
            curr->custom("dataset size: {}", records_.size(), phase);
            curr->custom("sort time: {}", instr::prettyDuration(timer.elapsed()));
        }

        for (auto&& [record, _] : records_) {
            if (!emit(phase, record.get())) {
                records_.clear();
                return false;
            }
        }

        records_.clear();
        return emit(phase, nullptr);
    }

    void init(int phase) override { source_->subscribe(phase, &sub_); }

    Key key(const Record& record) const {
        Key result;
        result.reserve(sort_list_.size());
        for (auto&& col : sort_list_) {
            result.push_back(col->eval(record));
        }
        return result;
    }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto source = source_->explain(ctx.withRequester(&sub_));

        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line(fullName()).child(source);
    }

    OperationPtr source_;
    bool desc_;
    SortList sort_list_;
    MemberSubscriber<Sort> sub_{this, &Sort::consume, prof_.inputHandle(&sub_)};

    // phase state
    int curr_phase_ = 0;
    std::vector<std::pair<ConstRecordPtr, Key>> records_;
};

OperationPtr sort(OperationPtr source, SortList glist, bool desc) {
    return std::make_shared<Sort>(std::move(source), desc, std::move(glist));
}

}  // namespace lsql::exec
