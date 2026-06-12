#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/op/MemberSubscriber.h"
#include "back/exec/op/OperationBase.h"

#include "util/instrument/Counters.h"
#include "util/verify.h"

namespace lsql::back::exec {

struct MarkJoinMetrics {
    void reset() { match_set_size.set(0); }
    util::StrBuilder report() const { return shortReport(); }

    util::StrBuilder shortReport() const {
        return util::StrBuilder("match_set_size: {}", match_set_size.value());
    }

    instr::Counter<size_t> match_set_size{0};
};

class MarkJoinRecord : public Record {
 public:
    MarkJoinRecord(Value value, FieldId id, RecordRef child)
        : child_(std::move(child))
        , id_(id)
        , value_(std::move(value)) {}

    const Value& value(FieldId id) const override {
        return id == id_ ? value_ : get(child_)->value(id);
    }

 private:
    std::shared_ptr<const Record> cloneImpl() const override {
        return std::make_shared<MarkJoinRecord>(value_, id_, pin(child_));
    }

    RecordRef child_;
    FieldId id_;
    Value value_;
};

class MarkJoin : public OperationBase<MarkJoin, MarkJoinMetrics> {
 public:
    MarkJoin(
        OperationPtr source,
        OperationPtr match_source,
        ScalarPtr proj,
        FieldId output_field_id,
        FieldId match_field_id,
        ConstFieldBindingPtr binding)
        : OperationBase(
              std::max(source->minPhase(), match_source->minPhase() + 1), std::move(binding))
        , source_(std::move(source))
        , match_source_(std::move(match_source))
        , proj_(std::move(proj))
        , output_field_id_(output_field_id)
        , match_field_id_(match_field_id) {
        prof::addEdge(sub_match_.scopeHandle(), prof_);
        prof::addEdge(sub_source_.scopeHandle(), prof_);
    }

 private:
    bool consumeMatch(int phase, const Record* record) {
        verify_dbg(phase == match_phase_);

        if (record == nullptr) {
            updateMetrics();
            // not emitting because it's not the last phase
            return false;
        }

        values_.insert(record->value(match_field_id_));

        if (!active(phase + 1)) {
            updateMetrics();
            return false;
        }

        return true;
    }

    bool consumeSource(int phase, const Record* record) {
        if (record == nullptr) {
            cleanIfDone(phase);
            return emit(phase, nullptr);
        }

        bool value = values_.contains(proj_->eval(*record));
        MarkJoinRecord marked_record(value, output_field_id_, record);

        if (!emit(phase, &marked_record)) {
            cleanIfDone(phase);
            return false;
        }

        return true;
    }

    void init(int out_phase, const FieldSet& downstream) override {
        verify(out_phase >= minPhase());

        if (match_phase_ == -1) {
            match_phase_ = out_phase - 1;
            match_source_->subscribe(
                out_phase - 1, &sub_match_, FieldSet::withField(match_field_id_));
        }

        // this may be an incorrect expectation
        verify(out_phase > match_phase_);

        source_->subscribe(
            out_phase, &sub_source_, FieldSet::merge(proj_->requiredFields(), downstream));
    }

    void updateMetrics() {
        if (auto m = prof_.metrics()) {
            m->custom<MarkJoinMetrics>().match_set_size.set(values_.size());
        }
    }

    void cleanIfDone(int phase) {
        updateMetrics();

        if (phase < maxPhase()) {
            return;
        }

        values_ = {};
    }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto match = match_source_->explain(ctx.withRequester(&sub_match_));
        auto source = source_->explain(ctx.withRequester(&sub_source_));

        if (ctx.phase < match_phase_) {
            // we do nothing on this phase
            verify(match.empty());
            verify(source.empty());
            return {};
        }

        if (ctx.phase == match_phase_) {
            verify(source.empty());

            auto item =
                ExplanationItem().line("{}: store match set", description(ctx.phase)).child(match);

            if (hasSubscriber(ctx.phase, ctx.requester)) {
                return item;
            } else {
                ctx.explanation.insert(item, this);
                return {};
            }
        }

        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem()
            .line("{}: stream + enrich with match", description(ctx.phase))
            .child(source);
    }

    OperationPtr source_;
    OperationPtr match_source_;
    ScalarPtr proj_;
    FieldId output_field_id_;
    FieldId match_field_id_;

    MemberSubscriber<MarkJoin> sub_source_{
        this,
        &MarkJoin::consumeSource,
        prof::newScope<ScopeMetrics>("{} src input", name()),
    };
    MemberSubscriber<MarkJoin> sub_match_{
        this,
        &MarkJoin::consumeMatch,
        prof::newScope<ScopeMetrics>("{} match set input", name()),
    };

    // phase at which values_ are built
    int match_phase_ = -1;
    std::unordered_set<Value> values_;
};

OperationPtr markJoin(
    OperationPtr source,
    OperationPtr match,
    ScalarPtr proj,
    FieldId output_field_id,
    FieldId match_field_id,
    ConstFieldBindingPtr binding) {
    return std::make_shared<MarkJoin>(
        std::move(source),
        std::move(match),
        std::move(proj),
        output_field_id,
        match_field_id,
        std::move(binding));
}

}  // namespace lsql::back::exec
