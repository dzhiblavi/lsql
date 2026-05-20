#pragma once

#include "exec/expr/Expression.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/OperationBase.h"

#include <vector>

namespace lsql::exec {

struct Projector {
    FieldId field_id;
    std::shared_ptr<Expression> expr;

    bool all() const { return expr == nullptr; }
};

using ProjectionList = std::vector<std::unique_ptr<Projector>>;
using ProjectionMap = std::unordered_map<FieldId, std::unique_ptr<Projector>>;

class ProjectionRecord : public Record {
 public:
    ProjectionRecord(RecordRef child, std::shared_ptr<const ProjectionMap> projectors, bool has_all)
        : child_(std::move(child))
        , projectors_(std::move(projectors))
        , has_all_(has_all) {}

    ids_t ids() const override {
        ids_t ids;
        if (has_all_) {
            ids = get(child_)->ids();
        }
        for (auto&& [id, _] : *projectors_) {
            ids.insert(id);
        }
        return ids;
    }

    Value value(FieldId id) const override {
        if (auto it = projectors_->find(id); it != projectors_->end()) {
            return it->second->expr->eval(*get(child_));
        }
        if (has_all_) {
            return get(child_)->value(id);
        }
        return null;
    }

    ConstRecordPtr cloneImpl() const override {
        return std::make_shared<ProjectionRecord>(pin(child_), projectors_, has_all_);
    }

 private:
    RecordRef child_;
    std::shared_ptr<const ProjectionMap> projectors_;
    const bool has_all_ = false;
};

class Projection : public OperationBase<Projection>,
                   public std::enable_shared_from_this<Projection> {
 public:
    Projection(OperationPtr source, ProjectionList projectors, ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , projectors_(buildProjectionMap(std::move(projectors))) {}

 private:
    bool consume(int phase, const Record* record) {
        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        ProjectionRecord rec(record, {shared_from_this(), &projectors_}, has_all_);
        return emit(phase, &rec);
    }

    void init(int phase, const RequiredFields& downstream) override {
        source_->subscribe(phase, &sub_, getRequiredFields(downstream));
    }

    RequiredFields getRequiredFields(const RequiredFields& downstream) const {
        RequiredFields result = RequiredFields::withNone();

        if (downstream.all()) {
            if (has_all_) {
                return RequiredFields::withAll();
            }

            for (auto&& [_, proj] : projectors_) {
                result.merge(proj->expr->requiredFields());
            }

            return result;
        }

        for (auto&& id : downstream.ids()) {
            if (auto it = projectors_.find(id); it != projectors_.end()) {
                result.merge(it->second->expr->requiredFields());
            } else if (has_all_) {
                result.require(id);
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
            .line("{} (*: {}, non-*: {})", description(ctx.phase), has_all_, projectors_.size())
            .child(source);
    }

    ProjectionMap buildProjectionMap(ProjectionList proj) {
        has_all_ = std::erase_if(proj, [](auto& p) { return p->all(); }) > 0;
        ProjectionMap res;
        res.reserve(proj.size());

        for (auto&& p : proj) {
            auto id = p->field_id;
            res.emplace(id, std::move(p));
        }

        return res;
    }

    OperationPtr source_;
    bool has_all_ = false;
    ProjectionMap projectors_;
    MemberSubscriber<Projection> sub_{
        this,
        &Projection::consume,
        prof_.inputHandle(&sub_),
    };
};

OperationPtr projection(OperationPtr source, ProjectionList slist, ConstFieldBindingPtr binding) {
    return std::make_shared<Projection>(std::move(source), std::move(slist), std::move(binding));
}

}  // namespace lsql::exec
