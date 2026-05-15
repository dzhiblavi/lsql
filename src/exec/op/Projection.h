#pragma once

#include "exec/expr/Expression.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/OperationBase.h"

#include <vector>

namespace lsql::exec {

struct Projector {
    std::string name;
    std::shared_ptr<Expression> expr;

    bool all() const { return expr == nullptr; }
};

using ProjectionList = std::vector<std::unique_ptr<Projector>>;
using ProjectionMap = std::unordered_map<std::string_view, std::unique_ptr<Projector>>;

class ProjectionRecord : public Record {
 public:
    ProjectionRecord(RecordRef child, std::shared_ptr<const ProjectionMap> projectors, bool has_all)
        : child_(std::move(child))
        , projectors_(std::move(projectors))
        , has_all_(has_all) {}

    values_t values() const override {
        values_t values;
        if (has_all_) {
            values = get(child_)->values();
        }
        for (auto&& [name, proj] : *projectors_) {
            values[std::string(name)] = proj->expr->eval(*get(child_));
        }
        return values;
    }

    Value value(std::string_view name) const override {
        if (auto it = projectors_->find(name); it != projectors_->end()) {
            return it->second->expr->eval(*get(child_));
        }
        if (has_all_) {
            return get(child_)->value(name);
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
    Projection(OperationPtr source, ProjectionList projectors)
        : OperationBase(source->minPhase())
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

        for (auto&& name : downstream.names()) {
            if (auto it = projectors_.find(name); it != projectors_.end()) {
                result.merge(it->second->expr->requiredFields());
            } else if (has_all_) {
                result.require(name);
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
            std::string_view name = p->name;
            res.emplace(name, std::move(p));
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

OperationPtr projection(OperationPtr source, ProjectionList slist) {
    return std::make_shared<Projection>(std::move(source), std::move(slist));
}

}  // namespace lsql::exec
