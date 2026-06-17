#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

#include "core/exceptions.h"
#include "util/instrument/Timer.h"

#include <llog/log.h>

#include <algorithm>
#include <vector>

namespace lsql::back::exec::phys {

struct SortMetrics {
    void reset() {
        dataset_size = 0;
        sort_time = {};
    }

    util::StrBuilder report() const { return shortReport(); }

    util::StrBuilder shortReport() const {
        return util::StrBuilder()
            .item("dataset_size: {}", dataset_size)
            .item("sort_time:    {}", instr::prettyDuration(sort_time));
    }

    size_t dataset_size{0};
    instr::MonotonicDuration sort_time{};
};

class Sort : public OperationBase<Sort, SortMetrics>, public std::enable_shared_from_this<Sort> {
    using Key = std::vector<Value>;

 public:
    Sort(int id, std::vector<Arc<Scalar>> sort_key, bool desc)
        : OperationBase(id)
        , sort_key_(std::move(sort_key))
        , desc_(desc) {
        require(!sort_key_.empty(), "sort list cannot be empty");
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record != nullptr) {
            records_.emplace_back(record->clone(), key(*record));

            if (!active()) {
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
            m->custom<SortMetrics>().dataset_size = records_.size();
            m->custom<SortMetrics>().sort_time = timer.elapsed();
        }

        for (auto&& [record, _] : records_) {
            if (!emit(record.get())) {
                records_.clear();
                return false;
            }
        }

        records_.clear();
        return emit(nullptr);
    }

    Key key(const Record& record) const {
        Key result;
        result.reserve(sort_key_.size());
        for (auto&& col : sort_key_) {
            result.push_back(col->eval(record));
        }
        return result;
    }

    std::vector<Arc<Scalar>> sort_key_;
    bool desc_;

    MemberSubscriber<Sort> sub_{
        this,
        &Sort::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };

    std::vector<std::pair<ConstRecordPtr, Key>> records_;
};

}  // namespace lsql::back::exec::phys
