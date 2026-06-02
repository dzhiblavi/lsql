#pragma once

#include "back/exec/op/MemberSubscriber.h"
#include "back/exec/op/OperationBase.h"
#include "back/exec/op/types.h"

#include "util/verify.h"

#include <llog/log.h>

#include <queue>
#include <vector>

namespace lsql::back::exec {

struct TopKMetrics {
    void reset() { seen_count = 0; }
    util::StrBuilder report() const { return shortReport(); }

    util::StrBuilder shortReport() const {
        return util::StrBuilder()
            .item("seen_count: {}", seen_count)
            .item("inserted_count: {}", inserted_count)
            .item("replaced_count: {}", replaced_count);
    }

    size_t seen_count{0};
    size_t inserted_count{0};
    size_t replaced_count{0};
};

class TopK : public OperationBase<TopK, TopKMetrics>, public std::enable_shared_from_this<TopK> {
    using Key = std::vector<Value>;

 public:
    TopK(
        OperationPtr source,
        int top_count,
        bool desc,
        SortList sort_list,
        ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , desc_(desc)
        , sort_list_(std::move(sort_list))
        , heap_(top_count, desc_) {
        require(!sort_list_.empty(), "ORDER BY list cannot be empty");
        prof::addEdge(&prof_sub_, &prof_);
    }

 private:
    bool consume(int phase, const Record* record) {
        if (curr_phase_ != phase) {
            curr_phase_ = phase;
            verify_dbg(heap_.empty());
        }

        if (record != nullptr) {
            if (!active(phase)) {
                heap_.clear();
                return false;
            }

            auto action = heap_.push(record, key(*record));

            if (auto m = prof_.metrics()) {
                auto& custom = m->custom<TopKMetrics>();
                ++custom.seen_count;

                switch (action) {
                    using enum BoundedPriorityQueue::Action;

                    case Inserted:
                        ++custom.inserted_count;
                        break;
                    case Replaced:
                        ++custom.replaced_count;
                        break;
                    case Skipped:
                        break;
                }
            }

            return true;
        }

        for (auto&& record : heap_.drain()) {
            if (!emit(phase, record.get())) {
                heap_.clear();
                return false;
            }
        }

        verify(heap_.empty());
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

        return ExplanationItem()
            .line("{} top_count={}, desc={}", description(ctx.phase), heap_.maxSize(), desc_)
            .child(source);
    }

    class BoundedPriorityQueue {
        struct Item {
            ConstRecordPtr record;
            Key key;
        };

        struct Comparator {
            bool desc;

            bool operator()(const Item& l, const Item& r) const {
                return desc ? r.key < l.key : l.key < r.key;
            }
        };

     public:
        using UnderlyingType = std::priority_queue<Item, std::vector<Item>, Comparator>;

        enum class Action : uint8_t {
            Inserted,
            Replaced,
            Skipped,
        };

        BoundedPriorityQueue(size_t max_size, bool desc)
            : desc_(desc)
            , max_size_(max_size)
            , heap_(Comparator(desc_), reservedVector()) {
            verify(max_size > 0);
        }

        Action push(const Record* record, Key key) {
            if (heap_.size() < max_size_) {
                heap_.push(
                    Item{
                        .record = record->clone(),
                        .key = std::move(key),
                    });
                return Action::Inserted;
            } else {
                auto&& top = heap_.top();  // worst kept element

                if (!better(key, top.key)) {
                    return Action::Skipped;
                }

                // key is better, should replace top
                heap_.pop();
                heap_.push(
                    Item{
                        .record = record->clone(),
                        .key = std::move(key),
                    });
                return Action::Replaced;
            }
        }

        void clear() { heap_ = UnderlyingType(Comparator(desc_), reservedVector()); }

        std::vector<ConstRecordPtr> drain() {
            std::vector<ConstRecordPtr> res;
            res.reserve(heap_.size());
            while (!heap_.empty()) {
                auto&& top = heap_.top();
                res.push_back(std::move(top.record));
                heap_.pop();
            }
            std::ranges::reverse(res);
            return res;
        }

        size_t maxSize() const { return max_size_; }

        bool empty() const { return heap_.empty(); }

     private:
        bool better(const Key& a, const Key& b) const { return desc_ ? b < a : a < b; }

        std::vector<Item> reservedVector() {
            std::vector<Item> v;
            v.reserve(max_size_);
            return v;
        }

        bool desc_;
        size_t max_size_;
        UnderlyingType heap_;
    };

    OperationPtr source_;
    bool desc_;
    SortList sort_list_;

    prof::ScopeHandle<ScopeMetrics> prof_sub_ = prof::newScope<ScopeMetrics>("{} input", name());
    MemberSubscriber<TopK> sub_{
        this,
        &TopK::consume,
        &prof_sub_,
    };

    // phase state
    int curr_phase_ = 0;
    BoundedPriorityQueue heap_;
};

OperationPtr topK(
    OperationPtr source,
    SortList sort_list,
    int top_count,
    bool desc,
    ConstFieldBindingPtr binding) {
    return std::make_shared<TopK>(
        std::move(source), top_count, desc, std::move(sort_list), std::move(binding));
}

}  // namespace lsql::back::exec
