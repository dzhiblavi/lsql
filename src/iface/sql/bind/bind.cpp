#include "iface/sql/bind/bind.h"

#include "iface/sql/ast/Expr.h"
#include "iface/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/ast/Relations.h"    // IWYU pragma: keep
#include "iface/sql/ast/Statement.h"    // IWYU pragma: keep

#include "iface/sql/bind/helpers.h"

#include "ir/Expressions.h"  // IWYU pragma: keep
#include "ir/Relations.h"    // IWYU pragma: keep

#include "core/time_formats.h"
#include "util/Pinned.h"

#include <magic_enum/magic_enum.hpp>

#include <algorithm>

namespace lsql::iface::sql::bind {

namespace {

ir::UnaryExprType exprType(ast::UnaryExprType ast) {
    switch (ast) {
        case ast::UnaryExprType::Not:
            return ir::UnaryExprType::BooleanNegate;
    }
}

ir::BinaryExprType exprType(ast::BinaryExprType ast) {
    switch (ast) {
        case ast::BinaryExprType::Equal:
            return ir::BinaryExprType::Equal;
        case ast::BinaryExprType::NotEqual:
            return ir::BinaryExprType::NotEqual;
        case ast::BinaryExprType::And:
            return ir::BinaryExprType::And;
        case ast::BinaryExprType::Or:
            return ir::BinaryExprType::Or;
        case ast::BinaryExprType::Divide:
            return ir::BinaryExprType::Divide;
    }
}

ValueType valueType(ValueType arg, ir::UnaryExprType type) {
    switch (type) {
        case ir::UnaryExprType::BooleanNegate:
            require(arg == ValueType::Boolean, "! argument should be boolean");
            return ValueType::Boolean;
    }
}

ValueType valueType(ValueType l, ValueType r, ir::BinaryExprType type) {
    switch (type) {
        case ir::BinaryExprType::Equal:
            require(
                l == r || l == ValueType::Null || r == ValueType::Null,
                "== arguments should have same type");
            return ValueType::Boolean;
        case ir::BinaryExprType::NotEqual:
            require(
                l == r || l == ValueType::Null || r == ValueType::Null,
                "!= arguments should have same type");
            return ValueType::Boolean;
        case ir::BinaryExprType::And:
            require(l == ValueType::Boolean, "AND arguments should be boolean");
            require(r == ValueType::Boolean, "AND arguments should be boolean");
            return ValueType::Boolean;
        case ir::BinaryExprType::Or:
            require(l == ValueType::Boolean, "OR arguments should be boolean");
            require(r == ValueType::Boolean, "OR arguments should be boolean");
            return ValueType::Boolean;
        case ir::BinaryExprType::Divide:
            require(l == r, "/ arguments should have same type");
            require(arithmetic(l), "/ arguments should be arithmetic");
            return l;
    }
}

bool isUnaryGroupFunc(std::string_view name) {
    static constexpr std::array<std::string_view, 4> Names{
        "builtin_count",
        "builtin_min",
        "builtin_max",
        "builtin_sum",
    };

    return std::find(Names.begin(), Names.end(), name) != Names.end();
}

ir::UnaryAggregateExprType unaryAggregateType(std::string_view func_name) {
    static constexpr std::array<std::pair<std::string_view, ir::UnaryAggregateExprType>, 4> Types{
        std::make_pair("builtin_count", ir::UnaryAggregateExprType::Count),
        std::make_pair("builtin_min", ir::UnaryAggregateExprType::Min),
        std::make_pair("builtin_max", ir::UnaryAggregateExprType::Max),
        std::make_pair("builtin_sum", ir::UnaryAggregateExprType::Sum),
    };

    auto it = std::ranges::find(Types, func_name, [](auto&& p) { return p.first; });
    verify(it != Types.end());
    return it->second;
}

ValueType valueType(ir::UnaryAggregateExprType type, ValueType arg) {
    switch (type) {
        case ir::UnaryAggregateExprType::Count:
            require(arg == ValueType::Boolean, "COUNT argument should be boolean");
            return ValueType::Integer;
        case ir::UnaryAggregateExprType::Min:
            return arg;
        case ir::UnaryAggregateExprType::Max:
            return arg;
        case ir::UnaryAggregateExprType::Sum:
            require(arithmetic(arg), "SUM argument should be arithmetic");
            return arg;
    }
}

class Binder {
 public:
    Binder() = default;

    ir::Program bind(ast::Program program) && {
        binding_ = std::make_shared<FieldBinding>();
        program_.field_binding = binding_;
        program_.statements.reserve(program.size());

        for (auto&& statement : program) {
            program_.statements.push_back(bindStatement(std::move(statement)));
        }

        return std::move(program_);
    }

 private:
    ir::Statement bindStatement(ast::Statement r) {
        return util::match(std::move(r), [this](auto r) { return bindStatement(std::move(r)); });
    }

    ir::Relation bindRelation(ast::Relation r) {
        return util::match(std::move(r), [this](auto r) { return bindRelation(std::move(r)); });
    }

    ir::Expr bindExpr(ast::Expr r) {
        return util::match(std::move(r), [this](auto r) { return bindExpr(std::move(r)); });
    }

    ir::Statement bindStatement(ast::QueryStatement s) {
        return ir::QueryStatement{
            .relation = std::make_unique<ir::Relation>(bindRelation(std::move(*s.relation))),
        };
    }

    ir::Statement bindStatement(ast::NamedRelationStatement s) {
        require(!named_relations_.contains(s.name), "duplicate named relation '{}'", s.name);

        auto relation = std::make_unique<ir::Relation>(bindRelation(std::move(*s.relation)));
        named_relations_[s.name] = relation.get();

        return ir::NamedRelationStatement{
            .name = s.name,
            .relation = std::move(relation),
        };
    }

    ir::Relation bindRelation(ast::AdhocRelation r) {
        std::vector<Value> values;
        values.reserve(r.literals.size());
        for (auto&& literal : r.literals) {
            values.push_back(parseLiteral(literal));
            require(
                values.back().type() == values.front().type(),
                "Ad hoc relation should contain entries of the same type");
        }

        auto type = values.empty() ? ValueType::Null : values.front().type();
        auto id = binding_->getOrAdd("anon", type);

        return ir::ValuesRelation{
            .values = std::move(values),
            .output_id = id,
            .fields_out = ir::RelationFields::withField(id),
        };
    }

    ir::Relation bindRelation(ast::SelectRelation r) {
        auto scope = scopeSource(bindRelation(std::move(*r.source)));

        if (r.where) {
            util::match(
                std::move(*r.where->condition),
                [&](ast::InExpr e) {
                    // Kind of an optimization over MarkJoinRelation
                    auto key = bindExpr(std::move(*e.expr));
                    require(
                        exprKindLevelOf(key) != ir::ExprKindLevel::Group,
                        "IN key expression cannot be aggregate");

                    auto match = bindRelation(std::move(*e.match));
                    auto source = pullSource();
                    auto fields = fieldsOutOf(source);

                    setSource(
                        ir::SemiJoinRelation{
                            .source = std::make_unique<ir::Relation>(std::move(source)),
                            .match = std::make_unique<ir::Relation>(std::move(match)),
                            .expr = std::make_unique<ir::Expr>(std::move(key)),
                            .fields_out = std::move(fields),
                        });
                },
                [&](auto e) {
                    auto cond = bindExpr(std::move(e));
                    auto source = pullSource();
                    auto fields = fieldsOutOf(source);

                    require(
                        valueTypeOf(cond) == ValueType::Boolean, "WHERE condition must be boolean");
                    require(
                        exprKindLevelOf(cond) != ir::ExprKindLevel::Group,
                        "WHERE condition cannot be aggregate");

                    setSource(
                        ir::FilterRelation{
                            .source = std::make_unique<ir::Relation>(std::move(source)),
                            .condition = std::make_unique<ir::Expr>(std::move(cond)),
                            .fields_out = std::move(fields),
                        });
                });
        }

        auto projectors = bindProjectors(std::move(r.projectors));
        require(!projectors.empty(), "SELECT requires at least one projector");

        bool has_group_projector = false;
        bool has_row_projector = false;
        bool has_group_by = r.group_by.has_value();
        for (auto&& p : projectors) {
            util::match(
                p,
                [&](const ir::StarProjector&) { has_row_projector = true; },
                [&](const ir::ExprProjector& p) {
                    has_group_projector |= exprKindLevelOf(*p.expr) == ir::ExprKindLevel::Group;
                    has_row_projector |= exprKindLevelOf(*p.expr) == ir::ExprKindLevel::Row;
                });
        }

        if (has_group_by) {
            auto group_by_list = bindProjectors(std::move(r.group_by->group_list));
            for (auto&& p : group_by_list) {
                util::matchPartial(
                    p,
                    [](const ir::StarProjector&) {
                        throwError("* is not allowed in GROUP BY statement");
                    },
                    [&](const ir::ExprProjector& p) {
                        require(
                            exprKindLevelOf(*p.expr) != ir::ExprKindLevel::Group,
                            "cannot aggregate aggregates");
                    });
            }

            auto fields = ir::RelationFields::merge(fieldsOf(group_by_list), fieldsOf(projectors));
            auto source = pullSource();

            setSource(
                ir::GroupRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .projectors = std::move(projectors),
                    .group_list = std::move(group_by_list),
                    .fields_out = std::move(fields),
                });
        } else if (has_group_projector) {
            require(!has_row_projector, "cannot mix group and row projectors");
            auto source = pullSource();
            auto fields = fieldsOf(projectors);

            setSource(
                ir::AggregateRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .projectors = std::move(projectors),
                    .fields_out = std::move(fields),
                });
        } else {
            auto source = pullSource();
            auto fields = fieldsOf(projectors);

            setSource(
                ir::ProjectionRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .projectors = std::move(projectors),
                    .fields_out = std::move(fields),
                });
        }

        if (r.order_by) {
            auto order_list = bindExprs(std::move(r.order_by->order_list));
            auto source = pullSource();
            auto fields = fieldsOutOf(source);

            setSource(
                ir::SortRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .order_list = std::move(order_list),
                    .desc = r.order_by->desc,
                    .fields_out = std::move(fields),
                });
        }

        if (r.limit) {
            auto source = pullSource();
            auto fields = fieldsOutOf(source);

            setSource(
                ir::LimitRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .limit = r.limit->limit,
                    .fields_out = std::move(fields),
                });
        }

        return pullSource();
    }

    ir::Relation bindRelation(ast::UnionAllRelation r) {
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));
        auto fields = ir::RelationFields::merge(fieldsOutOf(left), fieldsOutOf(right));

        return ir::UnionAllRelation{
            .left = std::make_unique<ir::Relation>(std::move(left)),
            .right = std::make_unique<ir::Relation>(std::move(right)),
            .fields_out = std::move(fields),
        };
    }

    ir::Relation bindRelation(ast::UnionAllSortedByRelation r) {
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));
        auto fields = ir::RelationFields::merge(fieldsOutOf(left), fieldsOutOf(right));

        setSource(
            ir::UnionAllSortedByRelation{
                .left = std::make_unique<ir::Relation>(std::move(left)),
                .right = std::make_unique<ir::Relation>(std::move(right)),
                .order_list = {},
                .desc = r.order_by.desc,
                .fields_out = std::move(fields),
            });

        auto* order_list = util::match(
            currSource(),
            [](ir::UnionAllSortedByRelation& r) -> std::vector<ir::Expr>* { return &r.order_list; },
            [](auto&&) -> std::vector<ir::Expr>* { panic(); });

        order_list->reserve(r.order_by.order_list.size());
        for (auto&& expr : r.order_by.order_list) {
            order_list->push_back(bindExpr(std::move(expr)));
        }
        require(!order_list->empty(), "order list cannot be empty");

        return pullSource();
    }

    ir::Relation bindRelation(ast::FileRelation r) {
        return ir::FileRelation{
            .path = std::move(r.path),
            .fields_out = ir::RelationFields::emptySet(),
        };
    }

    ir::Relation bindRelation(ast::FileIntervalRelation r) {
        constexpr auto format = TimeFormat::ISO8601;
        auto ts_from = timestampFromString(r.ts_from, format);

        return ir::FileIntervalRelation{
            .path = std::move(r.path),
            .ts_from = ts_from,
            .ts_to = ts_from + r.interval_s,
            .fields_out = ir::RelationFields::emptySet(),
        };
    }

    ir::Relation bindRelation(ast::NamedRelationReferenceRelation r) {
        auto it = named_relations_.find(r.name);
        require(it != named_relations_.end(), "no named relation '{}'", r.name);

        return ir::NamedRelationReferenceRelation{
            .name = std::move(r.name),
            .fields_out = fieldsOutOf(*it->second),
        };
    }

    ir::Relation bindRelation(ast::MaterializeRelation r) {
        auto arg = bindRelation(std::move(*r.relation));

        return ir::MaterializeRelation{
            .relation = std::make_unique<ir::Relation>(std::move(arg)),
            .fields_out = fieldsOutOf(arg),
        };
    }

    ir::Expr bindExpr(ast::IdentifierExpr e) {
        auto type = typeOfSourceField(e.identifier);
        auto id = binding_->getOrAdd(e.identifier, type);

        return ir::FieldExpr{
            .field_id = id,
            .type = type,
        };
    }

    ir::Expr bindExpr(ast::LiteralExpr e) {
        return ir::ValueExpr{
            .value = parseLiteral(e.literal),
        };
    }

    ir::Expr bindExpr(ast::CastExpr e) {
        return ir::CastExpr{
            .cast_to = e.cast_to,
            .expr = std::make_unique<ir::Expr>(bindExpr(std::move(*e.expr))),
        };
    }

    ir::Expr bindExpr(ast::InExpr e) {
        auto output_id = binding_->addAnonymous(ValueType::Boolean);

        auto expr = bindExpr(std::move(*e.expr));
        require(
            exprKindLevelOf(expr) != ir::ExprKindLevel::Group,
            "IN key expression cannot be aggregate");

        auto match = bindRelation(std::move(*e.match));
        auto fields = fieldsOutOf(currSource());
        fields.add(output_id);

        // same source relation enriched with a boolean field indicating
        // whether the row's key matches `match`
        setSource(
            ir::MarkJoinRelation{
                .source = std::make_unique<ir::Relation>(pullSource()),
                .match = std::make_unique<ir::Relation>(std::move(match)),
                .expr = std::make_unique<ir::Expr>(std::move(expr)),
                .output_field_id = output_id,
                .fields_out = std::move(fields),
            });

        return ir::FieldExpr{
            .field_id = output_id,
            .type = ValueType::Boolean,
        };
    }

    ir::Expr bindExpr(ast::LikeExpr e) {
        auto arg = bindExpr(std::move(*e.expr));
        require(valueTypeOf(arg) == ValueType::String, "LIKE argument should be String");

        return ir::LikeExpr{
            .expr = std::make_unique<ir::Expr>(std::move(arg)),
            .regex = std::move(e.regex),
        };
    }

    ir::Expr bindExpr(ast::FnCallExpr e) {
        std::vector<ir::Expr> args;
        args.reserve(e.args.size());
        for (auto&& arg : e.args) {
            args.push_back(bindExpr(std::move(arg)));
        }

        if (isUnaryGroupFunc(e.func)) {
            require(args.size() == 1, "function expects 1 argument");
            require(
                exprKindLevelOf(args[0]) != ir::ExprKindLevel::Group,
                "grouping operations do not accept aggregates");

            auto type = unaryAggregateType(e.func);
            auto value_type = valueType(type, valueTypeOf(args[0]));

            return ir::UnaryAggregateExpr{
                .type = type,
                .value_type = value_type,
                .expr = std::make_unique<ir::Expr>(std::move(args[0])),
            };
        }

        if (e.func == "builtin_coalesce") {
            require(args.size() >= 1, "at least one argument required for COALESCE");
            auto level = ir::ExprKindLevel::Const;
            for (auto&& arg : args) {
                require(
                    composable(level, exprKindLevelOf(arg)),
                    "different expression kinds not allowed in COALESCE");
                level = composed(level, exprKindLevelOf(arg));
            }

            std::unordered_set<ValueType> types;
            for (auto&& arg : args) {
                types.insert(valueTypeOf(arg));
            }
            require(types.size() == 1, "COALESCE arguments must have the same type");

            return ir::CoalesceExpr{
                .args = std::move(args),
            };
        }

        if (e.func == "builtin_percentile") {
            require(args.size() > 1, "PERCENTILE must be given at least one percentile");
            require(
                arithmetic(valueTypeOf(args[0])), "PERCENTILE first argument must be arithmetic");
            require(
                exprKindLevelOf(args[0]) != ir::ExprKindLevel::Group,
                "PERCENTILE does not accept aggregates");

            std::vector<float> percentiles;
            percentiles.reserve(args.size() - 1);
            for (size_t i = 1; i < args.size(); ++i) {
                percentiles.push_back(
                    util::match(
                        args[i],
                        [](ir::ValueExpr e) {
                            require(
                                e.value.type() == ValueType::Floating,
                                "PERCENTILE's arguments in positions >=1 must be floating");
                            float p = e.value.get<float>();
                            require(0.f <= p && p <= 1.f, "percentiles must be in [0.0, 1.0]");
                            return p;
                        },
                        [](auto&&) -> float {
                            throwError("PERCENTILE's arguments in positions >=1 must be literals");
                        }));
            }

            return ir::PercentileExpr{
                .expr = std::make_unique<ir::Expr>(std::move(args[0])),
                .percentiles = std::move(percentiles),
            };
        }

        if (e.func == "builtin_rsubstr") {
            require(args.size() == 2, "RSUBSTR expects exactly 2 arguments");
            require(
                valueTypeOf(args[0]) == ValueType::String,
                "RSUBSTR's first argument should be String");

            std::string regex = util::match(
                args[1],
                [](ir::ValueExpr e) {
                    require(
                        e.value.type() == ValueType::String,
                        "RSUBSTR's second argument should be String, got {}",
                        magic_enum::enum_name(e.value.type()));
                    return e.value.get<std::string>();
                },
                [](auto&&) -> std::string {
                    throwError("RSUBSTR's second argument should be a literal");
                });

            return ir::RSubstrExpr{
                .expr = std::make_unique<ir::Expr>(std::move(args[0])),
                .regex = std::move(regex),
            };
        }

        throwError("unknown function name {}", e.func);
    }

    ir::Expr bindExpr(ast::BinaryExpr e) {
        auto type = exprType(e.type);
        auto left = bindExpr(std::move(*e.left));
        auto right = bindExpr(std::move(*e.right));
        auto value_type = valueType(valueTypeOf(left), valueTypeOf(right), type);

        require(
            composable(exprKindLevelOf(left), exprKindLevelOf(right)),
            "Binary operations require same expression level on both sides");

        return ir::BinaryExpr{
            .type = type,
            .value_type = value_type,
            .left = std::make_unique<ir::Expr>(std::move(left)),
            .right = std::make_unique<ir::Expr>(std::move(right)),
        };
    }

    ir::Expr bindExpr(ast::UnaryExpr e) {
        auto arg = bindExpr(std::move(*e.expr));
        auto type = exprType(e.type);
        auto value_type = valueType(valueTypeOf(arg), type);

        return ir::UnaryExpr{
            .type = type,
            .value_type = value_type,
            .expr = std::make_unique<ir::Expr>(std::move(arg)),
        };
    }

    void bind(ast::Projector p, std::vector<ir::Projector>& out) {
        util::match(
            std::move(p),
            [&](ast::StarProjector) { out.emplace_back(ir::StarProjector{}); },
            [&](ast::IdentifierProjector p) {
                auto type = typeOfSourceField(p.identifier);
                auto id = binding_->getOrAdd(p.identifier, type);

                out.emplace_back(
                    ir::ExprProjector{
                        .alias_field_id = id,
                        .expr = std::make_unique<ir::Expr>(ir::FieldExpr{
                            .field_id = id,
                            .type = type,
                        }),
                    });
            },
            [&](ast::ExprProjector p) {
                auto expr = bindExpr(std::move(*p.expr));

                out.emplace_back(
                    ir::ExprProjector{
                        .alias_field_id = binding_->getOrAdd(p.alias, valueTypeOf(expr)),
                        .expr = std::make_unique<ir::Expr>(std::move(expr)),
                    });
            });
    }

    std::vector<ir::Projector> bindProjectors(std::vector<ast::Projector> projectors) {
        std::vector<ir::Projector> result;
        result.reserve(projectors.size());
        for (auto&& p : projectors) {
            bind(std::move(p), result);
        }
        return result;
    }

    std::vector<ir::Expr> bindExprs(std::vector<ast::Expr> exprs) {
        std::vector<ir::Expr> result;
        result.reserve(exprs.size());
        for (auto&& p : exprs) {
            result.push_back(bindExpr(std::move(p)));
        }
        return result;
    }

    struct ScopedCurrentSource : util::Pinned {
        std::optional<ir::Relation>* slot;
        std::optional<ir::Relation> old;

        ~ScopedCurrentSource() { *slot = std::move(old); }
    };

    ScopedCurrentSource scopeSource(ir::Relation current) {
        return ScopedCurrentSource{
            .slot = &current_source_slot_,
            .old = std::exchange(current_source_slot_, std::move(current)),
        };
    }

    void setSource(ir::Relation current) { current_source_slot_ = std::move(current); }

    ir::Relation& currSource() {
        verify(current_source_slot_.has_value());
        return *current_source_slot_;
    }

    ir::Relation pullSource() {
        verify(current_source_slot_.has_value());
        auto rel = std::move(*current_source_slot_);
        current_source_slot_ = std::nullopt;
        return rel;
    }

    ir::RelationFields fieldsOf(const ir::Projector& p) {
        return util::match(
            p,
            [](const ir::StarProjector&) { return ir::RelationFields::emptySet(); },
            [](const ir::ExprProjector& p) {
                return ir::RelationFields::withField(p.alias_field_id);
            });
    }

    ir::RelationFields fieldsOf(const std::vector<ir::Projector>& ps) {
        auto fields = ir::RelationFields::emptySet();
        for (auto&& p : ps) {
            fields.merge(fieldsOf(p));
        }
        return fields;
    }

    ValueType typeOfSourceField(std::string_view name) {
        auto&& fields = fieldsOutOf(currSource());

        for (auto id : fields.fieldIds()) {
            if (binding_->name(id) == name) {
                return binding_->type(id);
            }
        }

        return ValueType::String;
    }

    ir::Program program_;
    FieldBindingPtr binding_;
    std::unordered_map<std::string, ir::Relation*> named_relations_;
    std::optional<ir::Relation> current_source_slot_;
};

}  // namespace

ir::Program bind(ast::Program program) {
    return Binder().bind(std::move(program));
}

}  // namespace lsql::iface::sql::bind
