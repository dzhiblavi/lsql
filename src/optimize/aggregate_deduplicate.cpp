#include "optimize/aggregate_deduplicate.h"

#include "ir/Aggregates.h"
#include "ir/Relations.h"
#include "ir/Scalars.h"
#include "ir/tools.h"
#include "util/TreeWalker.h"
#include "util/deep_copy.h"

#include <unordered_map>

namespace lsql::opt {

namespace {

struct Optimizer : util::TreeWalker<Optimizer> {
    using Base = util::TreeWalker<Optimizer>;
    using Base::visit;

    Context& ctx;

    void deduplicate(auto& relation, ir::Relation& self) {
        if (relation.aggregates.size() < 2) {
            return;
        }

        std::unordered_map<ir::Aggregate, FieldId> expressions;
        std::vector<ir::Aggregate> aggregates;
        std::vector<ir::Projector> projectors;

        for (auto& aggregate : relation.aggregates) {
            auto key = util::deepCopy(aggregate);
            key.output_field_id = UnknownFieldId;

            auto [entry, inserted] =
                expressions.try_emplace(std::move(key), aggregate.output_field_id);

            projectors.push_back({
                .alias_field_id = aggregate.output_field_id,
                .expr =
                    box(ir::Scalar{
                        .node = ir::FieldScalar{.field_id = entry->second},
                        .value_type = aggregate.value_type,
                    }),
            });

            if (inserted) {
                aggregates.push_back(std::move(aggregate));
            }
        }

        auto removed = relation.aggregates.size() - aggregates.size();
        relation.aggregates = std::move(aggregates);
        if (removed == 0) {
            return;
        }

        auto schema = ir::schemaFor(relation.aggregates);

        if constexpr (requires { relation.group_list; }) {
            schema = Schema::concat(std::move(schema), ir::schemaFor(relation.group_list));
            for (const auto& key : relation.group_list) {
                projectors.push_back({
                    .alias_field_id = key.alias_field_id,
                    .expr =
                        box(ir::Scalar{
                            .node = ir::FieldScalar{.field_id = key.alias_field_id},
                            .value_type = key.expr->value_type,
                        }),
                });
            }
        }

        auto source =
            box(ir::Relation{
                .node = std::move(self.node),
                .schema = std::move(schema),
            });

        self.node = ir::ProjectionRelation{
            .source = std::move(source),
            .projectors = std::move(projectors),
        };

        ctx.setChanges().note("duplicate aggregates removed: {}", removed);
    }

    void visit(ir::Relation& self) {
        Base::visit(self);

        if (auto* aggregate = std::get_if<ir::AggregateRelation>(&self.node)) {
            deduplicate(*aggregate, self);
        } else if (auto* group = std::get_if<ir::GroupRelation>(&self.node)) {
            deduplicate(*group, self);
        }
    }
};

}  // namespace

ir::Program aggregateDeduplicate(ir::Program program, Context& ctx) {
    Optimizer optimizer{.ctx = ctx};
    for (auto& statement : program.statements) {
        optimizer.visit(statement);
    }
    return program;
}

}  // namespace lsql::opt
