#pragma once

#include "core/verify.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/OperationBase.h"
#include "exec/op/types.h"
#include "util/instrument/Timer.h"

#include <llog/log.h>

#include <algorithm>
#include <vector>

namespace lsql::exec {

class Sort : public OperationBase<Sort>, public std::enable_shared_from_this<Sort> {
    using Key = std::vector<Value>;

 public:
    Sort(OperationPtr source, bool desc, SortList sort_list)
        : OperationBase(source->minPhase(), "Sort")
        , source_(std::move(source))
        , desc_(desc)
        , sort_list_(std::move(sort_list)) {
        if (sort_list_.empty()) {
            throw std::runtime_error("ORDER BY list cannot be empty");
        }

        prof_.registerMetric(&dataset_size_);
        prof_.registerMetric(&sort_time_);
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

        if (prof_) {
            dataset_size_.counter.set(records_.size());
            sort_time_.set("sort time: {}", instr::prettyDuration(timer.elapsed()));
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

        return ExplanationItem().line("{} desc={}", name(), desc_).child(source);
    }

    prof::NamedCounter<size_t> dataset_size_{"dataset size", size_t(0)};
    prof::Message sort_time_;

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
