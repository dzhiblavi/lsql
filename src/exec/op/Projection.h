#pragma once

#include "exec/expr/Scalar.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/OperationBase.h"

#include <vector>

namespace lsql::exec {

struct ScalarProjector {
    FieldId field_id;
    ScalarPtr expr;
};

using ScalarProjectorPtr = std::unique_ptr<ScalarProjector>;
using ScalarProjectionList = std::vector<std::unique_ptr<ScalarProjector>>;
using ScalarProjectionMap = std::unordered_map<FieldId, std::unique_ptr<ScalarProjector>>;

class ScalarProjectionRecord : public Record {
 public:
    ScalarProjectionRecord(RecordRef child, std::shared_ptr<const ScalarProjectionMap> projectors)
        : child_(std::move(child))
        , projectors_(std::move(projectors)) {}

    ids_t ids() const override {
        ids_t ids;
        for (auto&& [id, _] : *projectors_) {
            ids.insert(id);
        }
        return ids;
    }

    Value value(FieldId id) const override {
        if (auto it = projectors_->find(id); it != projectors_->end()) {
            return it->second->expr->eval(*get(child_));
        }
        return null;
    }

    ConstRecordPtr cloneImpl() const override {
        return std::make_shared<ScalarProjectionRecord>(pin(child_), projectors_);
    }

 private:
    RecordRef child_;
    std::shared_ptr<const ScalarProjectionMap> projectors_;
};

class Projection : public OperationBase<Projection>,
                   public std::enable_shared_from_this<Projection> {
 public:
    Projection(OperationPtr source, ScalarProjectionList projectors, ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , projectors_(buildProjectionMap(std::move(projectors))) {}

 private:
    bool consume(int phase, const Record* record) {
        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        ScalarProjectionRecord rec(record, {shared_from_this(), &projectors_});
        return emit(phase, &rec);
    }

    void init(int phase, const FieldSet& downstream) override {
        source_->subscribe(phase, &sub_, getFieldSet(downstream));
    }

    FieldSet getFieldSet(const FieldSet& downstream) const {
        FieldSet result = FieldSet::emptySet();

        for (auto&& id : downstream.fieldIds()) {
            if (auto it = projectors_.find(id); it != projectors_.end()) {
                result.merge(it->second->expr->requiredFields());
            }
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
            .line("{} (projectors: {})", description(ctx.phase), projectors_.size())
            .child(source);
    }

    ScalarProjectionMap buildProjectionMap(ScalarProjectionList proj) {
        ScalarProjectionMap res;
        res.reserve(proj.size());

        for (auto&& p : proj) {
            auto id = p->field_id;
            res.emplace(id, std::move(p));
        }

        return res;
    }

    OperationPtr source_;
    ScalarProjectionMap projectors_;
    MemberSubscriber<Projection> sub_{
        this,
        &Projection::consume,
        prof_.inputHandle(&sub_),
    };
};

OperationPtr projection(
    OperationPtr source, ScalarProjectionList slist, ConstFieldBindingPtr binding) {
    return std::make_shared<Projection>(std::move(source), std::move(slist), std::move(binding));
}

}  // namespace lsql::exec
