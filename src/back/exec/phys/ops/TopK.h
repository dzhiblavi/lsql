#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

#include "core/exceptions.h"
#include "util/verify.h"

#include <queue>
#include <vector>

namespace lsql::back::exec::phys {

struct TopKMetrics {
    void reset() {
        seen_count = 0;
        inserted_count = 0;
        replaced_count = 0;
    }

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
    TopK(int id, std::vector<Arc<Scalar>> sort_key, int top_count, bool desc)
        : OperationBase(id)
        , sort_key_(std::move(sort_key))
        , desc_(desc)
        , heap_(top_count, desc_) {
        require(!sort_key_.empty(), "topk list cannot be empty");
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record != nullptr) {
            if (!active()) {
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
            if (!emit(record.get())) {
                heap_.clear();
                return false;
            }
        }

        verify(heap_.empty());
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

    std::vector<Arc<Scalar>> sort_key_;
    bool desc_;

    MemberSubscriber<TopK> sub_{
        this,
        &TopK::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };

    BoundedPriorityQueue heap_;
};

}  // namespace lsql::back::exec::phys
