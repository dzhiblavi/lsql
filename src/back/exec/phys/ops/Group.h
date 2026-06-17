#pragma once

#include "back/exec/Record.h"
#include "back/exec/expr/Aggregate.h"
#include "back/exec/expr/Scalar.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

#include "core/types.h"
#include "util/containers.h"

#include <vector>

namespace lsql::back::exec::phys {

class Group : public OperationBase<Group>, public std::enable_shared_from_this<Group> {
 public:
    Group(int id, std::vector<Arc<Aggregate>> aggregates, std::vector<Arc<Scalar>> group_key)
        : OperationBase(id)
        , aggregates_(std::move(aggregates))
        , group_key_(std::move(group_key)) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record != nullptr) {
            auto key = getKey(*record);

            auto&& aggregators = groups_[key];
            if (aggregators.empty()) {
                for (auto&& aggregate : aggregates_) {
                    if (aggregate) {
                        aggregators.push_back(aggregate->aggregator());
                    } else {
                        aggregators.emplace_back();
                    }
                }
            }

            for (auto&& aggregator : aggregators) {
                if (aggregator != nullptr) {
                    aggregator->feed(*record);
                }
            }

            if (!active()) {
                groups_.clear();
                return false;
            }

            return true;
        }

        // end of stream
        while (!groups_.empty()) {
            auto node = groups_.extract(groups_.begin());
            auto key = std::move(node.key());

            std::vector<Value> values;
            values.reserve(aggregates_.size() + group_key_.size());

            // schema: <aggregates...>, <group key...>
            auto aggregators = std::move(node.mapped());
            for (auto&& aggregator : aggregators) {
                if (aggregator != nullptr) {
                    values.push_back(aggregator->get());
                } else {
                    values.emplace_back(null);
                }
            }

            util::append(values, std::move(key));
            auto record = arc<VecRecord>(std::move(values));

            if (!emit(record.get())) {
                groups_.clear();
                return false;
            }
        }

        return emit(nullptr);
    }

    std::vector<Value> getKey(const Record& record) {
        std::vector<Value> key;
        key.reserve(group_key_.size());
        for (auto&& scalar : group_key_) {
            key.push_back(scalar->eval(record));
        }
        return key;
    }

    std::vector<Arc<Aggregate>> aggregates_;
    std::vector<Arc<Scalar>> group_key_;
    MemberSubscriber<Group> sub_{
        this,
        &Group::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };

    using Groups = std::unordered_map<std::vector<Value>, std::vector<Arc<Aggregator>>>;
    Groups groups_;
};

}  // namespace lsql::back::exec::phys
