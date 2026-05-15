#pragma once

#include "core/verify.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/OperationBase.h"
#include "exec/op/types.h"

#include <array>
#include <queue>

namespace lsql::exec {

class MergeSorted : public OperationBase<MergeSorted> {
    enum class DrainResult : uint8_t {
        Continue,
        StopRequested,
    };

 public:
    MergeSorted(OperationPtr l, OperationPtr r, SortList slist, bool desc)
        : OperationBase(std::max(l->minPhase(), r->minPhase()))
        , l_(std::move(l))
        , r_(std::move(r))
        , slist_(std::move(slist))
        , desc_(desc) {
        prof_.registerMetric(&buf_sizes_[0]);
        prof_.registerMetric(&buf_sizes_[1]);
    }

 private:
    // Subscriber
    template <int Index>
    bool consume(int phase, const Record* record) {
        static_assert(Index == 0 || Index == 1);

        if (delay_done_) {
            verify(eof_[1 - Index]);
            reset();
            return false;
        }

        verify(!eof_[Index]);

        if (record == nullptr) {
            eof_[Index] = true;

            if (drain(phase) == DrainResult::StopRequested) {
                terminate(Index);
            } else {
                if (eof()) {
                    emitLast(phase);
                    reset();
                }
            }

            // false anyhow, no more input from this side
            return false;
        }

        verify(record != nullptr);

        if (shouldWait(1 - Index)) {
            // waiting for records from other side to compare to, cannot emit
            push(Index, record);
            return true;
        }

        if (!buffers_[Index].empty()) {
            // already buffering this side
            push(Index, record);

            if (drain(phase) == DrainResult::Continue) {
                return true;
            }

            // StopRequested, no emit last
            terminate(Index);
            return false;
        }

        // current side's buffer is empty,
        // other side is not in waiting state (either has records or eof)
        if (drain(phase, Index, record) == DrainResult::StopRequested) {
            terminate(Index);
            return false;
        }

        return true;
    }

    // Operation
    void init(int phase) override {
        l_->subscribe(phase, &sub_l_);
        r_->subscribe(phase, &sub_r_);
    }

    bool shouldWait(int index) const {
        // buffer is empty and not eof yet
        return !eof_[index] && buffers_[index].empty();
    }

    void push(int index, const Record* record) {
        buffers_[index].emplace(record->clone(), key(*record));

        if (prof_) {
            buf_sizes_[index].counter.max(buffers_[index].size());
        }
    }

    // drain from buffers only
    DrainResult drain(int phase) {
        verify(!delay_done_);

        auto& lb = buffers_[Left];
        auto& rb = buffers_[Right];

        while (!lb.empty() && !rb.empty()) {
            const auto& [l, lkey] = lb.front();
            const auto& [r, rkey] = rb.front();

            int curr = less(lkey, rkey) ? Left : Right;
            auto record = pop(curr);

            if (!emit(phase, record.get())) {
                return DrainResult::StopRequested;
            }
        }

        if (eof_[Right] && !lb.empty()) {
            verify(rb.empty());
            return drainSide(phase, Left);
        }

        if (eof_[Left] && !rb.empty()) {
            verify(lb.empty());
            return drainSide(phase, Right);
        }

        return DrainResult::Continue;
    }

    DrainResult drainSide(int phase, int side) {
        verify(eof_[1 - side]);
        verify(buffers_[1 - side].empty());
        verify(!delay_done_);

        // "other" will not push more records, so drain the buffer
        while (!buffers_[side].empty()) {
            auto record = pop(side);

            if (!emit(phase, record.get())) {
                return DrainResult::StopRequested;
            }
        }

        return DrainResult::Continue;
    }

    DrainResult drain(int phase, int curr_side, const Record* record) {
        verify(record != nullptr);
        verify(buffers_[curr_side].empty());

        auto k = key(*record);
        auto& b = buffers_[1 - curr_side];  // other side's buffer

        while (!b.empty()) {
            auto& [v, kv] = b.front();

            if (less(k, kv)) {
                if (!emit(phase, record)) {
                    return DrainResult::StopRequested;
                }

                // now we do not have records to compare to
                return DrainResult::Continue;
            } else {
                auto v = pop(1 - curr_side);

                if (!emit(phase, v.get())) {
                    // simply drop record because no more output is needed
                    return DrainResult::StopRequested;
                }
            }
        }

        // other buffer is drained
        // need to push record to be processed later
        push(curr_side, record);
        return DrainResult::Continue;
    }

    bool less(const SortKey& a, const SortKey& b) { return desc_ ^ (a < b); }

    bool eof() const { return eof_[Left] && eof_[Right]; }

    void terminate(int stopped_side) {
        eof_[stopped_side] = true;

        if (eof()) {
            // both sides reached eof, termination is complete
            reset();
        } else {
            // request other side to stop. it will handle state reset
            delay_done_ = true;
        }
    }

    void emitLast(int phase) { emit(phase, nullptr); }

    void reset() {
        buffers_[Left] = buffers_[Right] = {};
        eof_[Left] = eof_[Right] = false;
        delay_done_ = false;
    }

    ConstRecordPtr pop(int index) {
        verify(!buffers_[index].empty());
        auto [val, _] = std::move(buffers_[index].front());
        buffers_[index].pop();
        return val;
    }

    SortKey key(const Record& record) const {
        SortKey result;
        result.reserve(slist_.size());
        for (auto&& col : slist_) {
            result.push_back(col->eval(record));
        }
        return result;
    }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto l = l_->explain(ctx.withRequester(&sub_l_));
        auto r = r_->explain(ctx.withRequester(&sub_r_));

        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem()
            .line("{} cols={} desc={}", name(), slist_.size(), desc_)
            .child(l)
            .child(r);
    }

    static constexpr int Left = 0;
    static constexpr int Right = 1;

    std::array<prof::NamedCounter<size_t>, 2> buf_sizes_{
        prof::NamedCounter("max left buf size", size_t(0)),
        prof::NamedCounter("max right buf size", size_t(0)),
    };

    OperationPtr l_;
    OperationPtr r_;
    SortList slist_;
    bool desc_;

    std::mutex m_;
    MemberSubscriber<MergeSorted, LockMixin> sub_l_{
        this,
        &MergeSorted::consume<0>,
        &m_,
        prof_.inputHandle(&sub_l_),
    };
    MemberSubscriber<MergeSorted, LockMixin> sub_r_{
        this,
        &MergeSorted::consume<1>,
        &m_,
        prof_.inputHandle(&sub_r_),
    };

    bool delay_done_ = false;
    std::array<bool, 2> eof_ = {false};
    std::array<std::queue<std::pair<ConstRecordPtr, SortKey>>, 2> buffers_ = {};
};

OperationPtr mergeSorted(OperationPtr l, OperationPtr r, SortList slist, bool desc) {
    return std::make_shared<MergeSorted>(std::move(l), std::move(r), std::move(slist), desc);
}

}  // namespace lsql::exec
