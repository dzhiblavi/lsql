#pragma once

#include "exec/op/MemberSubscriber.h"
#include "exec/op/OperationBase.h"
#include "exec/op/types.h"

#include "core/verify.h"
#include "util/instrument/Counters.h"
#include "util/instrument/Timer.h"

#include <llog/log.h>

#include <algorithm>
#include <vector>

namespace lsql::exec {

struct SortCustomMetrics {
    instr::Counter<size_t> dataset_size{0};
    instr::MonotonicDuration sort_time{};

    void reset() {
        dataset_size.set(0);
        sort_time = {};
    }

    util::StrBuilder format() const {
        return util::StrBuilder()
            .item("dataset_size: {}", dataset_size.value())
            .item("sort_time:    {}", instr::prettyDuration(sort_time));
    }
};

class Sort : public OperationBase<Sort, SortCustomMetrics>,
             public std::enable_shared_from_this<Sort> {
    using Key = std::vector<Value>;

 public:
    Sort(OperationPtr source, bool desc, SortList sort_list, ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , desc_(desc)
        , sort_list_(std::move(sort_list)) {
        require(!sort_list_.empty(), "ORDER BY list cannot be empty");
        prof::addEdge(&prof_sub_, &prof_);
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

        if (auto m = prof_.metrics()) {
            m->custom.dataset_size.set(records_.size());
            m->custom.sort_time = timer.elapsed();
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

    void init(int phase, const FieldSet& fields) override {
        source_->subscribe(phase, &sub_, getFieldSet(fields));
    }

    FieldSet getFieldSet(const FieldSet& downstream) const {
        FieldSet result = downstream;

        for (auto&& proj : sort_list_) {
            result.merge(proj->requiredFields());
        }

        return result;
    }

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

        return ExplanationItem().line("{} desc={}", description(ctx.phase), desc_).child(source);
    }

    OperationPtr source_;
    bool desc_;
    SortList sort_list_;

    prof::ScopeHandle<ScopeMetrics<>> prof_sub_ =
        prof::newScope<ScopeMetrics<>>("{} input", name());
    MemberSubscriber<Sort> sub_{
        this,
        &Sort::consume,
        &prof_sub_,
    };

    // phase state
    int curr_phase_ = 0;
    std::vector<std::pair<ConstRecordPtr, Key>> records_;
};

OperationPtr sort(OperationPtr source, SortList glist, bool desc, ConstFieldBindingPtr binding) {
    return std::make_shared<Sort>(std::move(source), desc, std::move(glist), std::move(binding));
}

}  // namespace lsql::exec
