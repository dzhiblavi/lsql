#include "iface/sql/bind/bind.h"

#include "iface/sql/ast/Expr.h"
#include "iface/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/ast/Relations.h"    // IWYU pragma: keep
#include "iface/sql/ast/Statement.h"    // IWYU pragma: keep

#include "iface/sql/bind/Expressions.h"
#include "iface/sql/bind/Relations.h"
#include "iface/sql/bind/helpers.h"

#include "core/expressions.h"
#include "core/time_formats.h"

#include "util/Pinned.h"

#include <magic_enum/magic_enum.hpp>

#include <algorithm>

namespace lsql::iface::sql::bind {

namespace {

bool arithmetic(ValueType type) {
    switch (type) {
        case ValueType::String:
        case ValueType::Boolean:
        case ValueType::Null:
            return false;

        case ValueType::Integer:
        case ValueType::Floating:
            return true;
    }
}

UnaryExprType exprType(ast::UnaryExprType ast) {
    switch (ast) {
        case ast::UnaryExprType::Not:
            return UnaryExprType::BooleanNegate;
    }
}

BinaryExprType exprType(ast::BinaryExprType ast) {
    switch (ast) {
        case ast::BinaryExprType::Equal:
            return BinaryExprType::Equal;
        case ast::BinaryExprType::NotEqual:
            return BinaryExprType::NotEqual;
        case ast::BinaryExprType::And:
            return BinaryExprType::And;
        case ast::BinaryExprType::Or:
            return BinaryExprType::Or;
        case ast::BinaryExprType::Divide:
            return BinaryExprType::Divide;
        case ast::BinaryExprType::Plus:
            return BinaryExprType::Add;
        case ast::BinaryExprType::Minus:
            return BinaryExprType::Subtract;
    }
}

ValueType valueType(ValueType arg, UnaryExprType type) {
    switch (type) {
        case UnaryExprType::BooleanNegate:
            require(arg == ValueType::Boolean, "! argument should be boolean");
            return ValueType::Boolean;
    }
}

ValueType valueType(ValueType l, ValueType r, BinaryExprType type) {
    switch (type) {
        case BinaryExprType::Equal:
            require(
                l == r || l == ValueType::Null || r == ValueType::Null,
                "== arguments should have same type");
            return ValueType::Boolean;
        case BinaryExprType::NotEqual:
            require(
                l == r || l == ValueType::Null || r == ValueType::Null,
                "!= arguments should have same type");
            return ValueType::Boolean;
        case BinaryExprType::And:
            require(l == ValueType::Boolean, "AND arguments should be boolean");
            require(r == ValueType::Boolean, "AND arguments should be boolean");
            return ValueType::Boolean;
        case BinaryExprType::Or:
            require(l == ValueType::Boolean, "OR arguments should be boolean");
            require(r == ValueType::Boolean, "OR arguments should be boolean");
            return ValueType::Boolean;
        case BinaryExprType::Divide:
            require(l == r, "/ arguments should have same type");
            require(arithmetic(l), "/ arguments should be arithmetic");
            return l;
        case BinaryExprType::Add:
        case BinaryExprType::Subtract:
            require(l == r, "+/- arguments should have same type");
            // TODO
            return l;
    }
}

std::optional<UnaryAggregateType> unaryAggregateExprType(std::string_view fn_name) {
    static constexpr std::array<std::pair<std::string_view, UnaryAggregateType>, 4> Types{
        std::make_pair("builtin_count", UnaryAggregateType::Count),
        std::make_pair("builtin_min", UnaryAggregateType::Min),
        std::make_pair("builtin_max", UnaryAggregateType::Max),
        std::make_pair("builtin_sum", UnaryAggregateType::Sum),
    };

    auto it = std::ranges::find(Types, fn_name, [](auto&& p) { return p.first; });
    return it == Types.end() ? std::nullopt : std::optional(it->second);
}

ValueType unaryAggregateValueType(UnaryAggregateType type, ValueType arg) {
    switch (type) {
        case UnaryAggregateType::Count:
            require(arg == ValueType::Boolean, "COUNT argument should be boolean");
            return ValueType::Integer;
        case UnaryAggregateType::Min:
        case UnaryAggregateType::Max:
            return arg;
            return arg;
        case UnaryAggregateType::Sum:
            require(arithmetic(arg), "SUM argument should be arithmetic");
            return arg;
        default:
            throwError("not an aggregate");
    }
}

class Binder {
 public:
    Binder() = default;

    Program bind(ast::Program program) && {
        binding_ = std::make_shared<FieldBinding>();
        program_.binding = binding_;
        program_.statements.reserve(program.size());

        for (auto&& statement : program) {
            program_.statements.push_back(bindStatement(std::move(statement)));
        }

        return std::move(program_);
    }

 private:
    Statement bindStatement(ast::Statement r) {
        return util::match(std::move(r), [this](auto r) { return bindStatement(std::move(r)); });
    }

    Relation bindRelation(ast::Relation r) {
        return util::match(std::move(r), [this](auto r) { return bindRelation(std::move(r)); });
    }

    Expr bindExpr(ast::Expr r) {
        return util::match(std::move(r), [this](auto r) { return bindExpr(std::move(r)); });
    }

    Statement bindStatement(ast::QueryStatement s) {
        return QueryStatement{
            .relation = box(bindRelation(std::move(*s.relation))),
        };
    }

    Statement bindStatement(ast::NamedRelationStatement s) {
        require(!named_relations_.contains(s.name), "duplicate named relation '{}'", s.name);

        auto relation = box(bindRelation(std::move(*s.relation)));
        named_relations_[s.name] = relation.get();

        return NamedRelationStatement{
            .name = s.name,
            .relation = std::move(relation),
        };
    }

    Relation bindRelation(ast::AdhocRelation r) {
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

        return {
            .node =
                AdhocRelation{
                    .values = std::move(values),
                    .output_field_id = id,
                },
            .fields_out = FieldSetNode::make(FieldSet::withField(id)),
        };
    }

    Relation bindRelation(ast::SelectRelation r) {
        auto source = bindRelation(std::move(*r.source));
        auto source_field_set_node = source.fields_out;

        auto generated_visible_fields = FieldSetNode::emptySet();
        auto source_visible_fields = FieldSetChain(source_field_set_node.get(), nullptr);
        auto visible_fields = FieldSetChain(generated_visible_fields.get(), &source_visible_fields);
        auto _ = scopedFieldSet(&visible_fields);

        std::optional<Where> where;
        if (r.where) {
            auto cond = bindExpr(std::move(*r.where->condition));
            require(cond.value_type == ValueType::Boolean, "WHERE condition must be boolean");
            require(cond.level != ExprKindLevel::Group, "WHERE condition cannot be aggregate");
            where = Where{.condition = box<Expr>(std::move(cond))};
        }

        std::optional<Limit> limit;
        if (r.limit) {
            require(r.limit->limit > 0, "limit cannot be negative");
            limit = Limit{.limit = r.limit->limit};
        }

        auto projectors = bindProjectors(std::move(r.projectors));
        require(!projectors.empty(), "SELECT requires at least one projector");
        auto output_fields = outputFieldsOf(projectors);
        FieldSetNodePtr fields_out;

        bool has_group_by = r.group_by.has_value();
        bool has_group_projector = false;
        for (auto&& p : projectors) {
            util::match(
                p,
                [&](const ExprProjector& p) {
                    has_group_projector |= p.expr->level == ExprKindLevel::Group;
                },
                [&](const IdentifierProjector&) {},
                [&](const StarProjector&) {});
        }

        std::optional<GroupBy> group_by;
        if (has_group_by) {
            // Group by
            auto group_key = bindProjectors(std::move(r.group_by->group_list));

            for (auto&& p : group_key) {
                util::matchPartial(p, [](const StarProjector&) {
                    throwError("Star projectors are not allowed in GROUP BY");
                });
            }

            auto group_key_map = buildMap(group_key);

            for (auto&& p : projectors) {
                util::match(
                    p,
                    [](const StarProjector&) { /* ok, all group keys */ },
                    [&](const IdentifierProjector& p) {
                        require(
                            group_key_map.contains(p.field_id),
                            "GROUP BY: unknown field id {}",
                            p.field_id);
                    },
                    [&](const ExprProjector& p) {
                        if (p.expr->level != ExprKindLevel::Row) {
                            // Ok (Const/Group projectors)
                            return;
                        }
                        for (auto id : p.expr->required_fields.fieldIds()) {
                            require(
                                group_key_map.contains(id),
                                "GROUP BY: unknown field id {} required by expression",
                                id);
                        }
                    });
            }

            group_by = GroupBy{.group_list = std::move(group_key)};

            // Add group output keys
            generated_visible_fields->merge(outputFieldsOf(group_key));
            fields_out = FieldSetNode::make(output_fields);
        } else if (has_group_projector) {
            // Aggregate
            for (auto&& p : projectors) {
                util::match(
                    p,
                    [](const StarProjector&) {
                        throwError("Star projectors are not allowed in aggregates");
                    },
                    [](const IdentifierProjector&) {
                        throwError("Identifier projectors are not allowed in aggregates");
                    },
                    [](const ExprProjector& p) {
                        require(
                            p.expr->level != ExprKindLevel::Row,
                            "Row projectors are not allowed in aggregates");
                    });
            }
            fields_out = FieldSetNode::make(output_fields);
        } else {
            // Simple select, nothing left to check
            fields_out = FieldSetNode::make(output_fields, source_field_set_node);
        }

        std::optional<OrderBy> order_by;
        if (r.order_by) {
            generated_visible_fields->merge(output_fields);

            auto order_list = bindExprs(std::move(r.order_by->order_list));
            order_by = OrderBy{
                .order_list = std::move(order_list),
                .desc = r.order_by->desc,
            };
        }

        return {
            .node =
                SelectRelation{
                    .projectors = std::move(projectors),
                    .source = box(std::move(source)),
                    .limit = std::move(limit),
                    .where = std::move(where),
                    .order_by = std::move(order_by),
                    .group_by = std::move(group_by),
                    .aggregate = has_group_projector,
                },
            .fields_out = std::move(fields_out),
        };
    }

    Relation bindRelation(ast::UnionAllRelation r) {
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));
        auto fields = FieldSetNode::proxy(left.fields_out, right.fields_out);

        return {
            .node =
                UnionAllRelation{
                    .left = box(std::move(left)),
                    .right = box(std::move(right)),
                },
            .fields_out = std::move(fields),
        };
    }

    Relation bindRelation(ast::UnionAllSortedByRelation r) {
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));
        auto fields = FieldSetNode::proxy(left.fields_out, right.fields_out);

        FieldSetChain fields_set(fields.get(), nullptr);
        auto _ = scopedFieldSet(&fields_set);

        auto order_list = bindExprs(std::move(r.order_by.order_list));
        require(!order_list.empty(), "order list cannot be empty");

        return {
            .node =
                UnionAllSortedByRelation{
                    .left = box(std::move(left)),
                    .right = box(std::move(right)),
                    .order_by =
                        OrderBy{
                            .order_list = std::move(order_list),
                            .desc = r.order_by.desc,
                        },
                },
            .fields_out = std::move(fields),
        };
    }

    Relation bindRelation(ast::FileRelation r) {
        return {
            .node =
                FileRelation{
                    .path = std::move(r.path),
                },
            .fields_out = FieldSetNode::unknownSet(),
        };
    }

    Relation bindRelation(ast::FileIntervalRelation r) {
        constexpr auto format = TimeFormat::ISO8601;
        auto ts_from = timestampFromString(r.ts_from, format);

        return {
            .node =
                FileIntervalRelation{
                    .path = std::move(r.path),
                    .ts_from = ts_from,
                    .ts_to = ts_from + r.interval_s,
                },
            .fields_out = FieldSetNode::unknownSet(),
        };
    }

    Relation bindRelation(ast::NamedRelationReferenceRelation r) {
        auto it = named_relations_.find(r.name);
        require(it != named_relations_.end(), "no named relation '{}'", r.name);
        auto child_node_ptr = it->second->fields_out;

        return {
            .node = NamedRelationReferenceRelation{.name = std::move(r.name)},
            .fields_out = FieldSetNode::proxy(child_node_ptr),
        };
    }

    Relation bindRelation(ast::MaterializeRelation r) {
        auto arg = bindRelation(std::move(*r.relation));
        auto fields = FieldSetNode::proxy(arg.fields_out);

        return {
            .node = MaterializeRelation{.relation = box(std::move(arg))},
            .fields_out = std::move(fields),
        };
    }

    Expr bindExpr(ast::IdentifierExpr e) {
        auto type = currFieldSet().typeOfSourceField(e.identifier, binding_);
        auto id = binding_->getOrAdd(e.identifier, type);

        return {
            .node = IdentifierExpr{.field_id = id},
            .value_type = type,
            .level = ExprKindLevel::Row,
            .required_fields = FieldSet::withField(id),
        };
    }

    Expr bindExpr(ast::LiteralExpr e) {
        auto value = parseLiteral(e.literal);

        return {
            .node = ValueExpr{.value = value},
            .value_type = value.type(),
            .level = ExprKindLevel::Const,
            .required_fields = FieldSet::emptySet(),
        };
    }

    Expr bindExpr(ast::CastExpr e) {
        auto expr = bindExpr(std::move(*e.expr));
        auto type = e.cast_to;
        auto level = expr.level;
        auto req_fields = expr.required_fields;

        return {
            .node =
                CastExpr{
                    .cast_to = e.cast_to,
                    .expr = box(std::move(expr)),
                },
            .value_type = type,
            .level = level,
            .required_fields = req_fields,
        };
    }

    Expr bindExpr(ast::InExpr e) {
        auto expr = bindExpr(std::move(*e.expr));
        require(expr.level != ExprKindLevel::Group, "IN key expression cannot be aggregate");
        auto req_fields = expr.required_fields;

        Relation match = bindRelation(std::move(*e.match));
        auto count = match.fields_out->fieldSet().fieldIds().size();
        verify(count == 1, "expected 1, got {}", count);
        require(count == 1, "IN match should contain one column, got {}", count);

        auto match_field_id = *match.fields_out->fieldSet().fieldIds().begin();

        return {
            .node =
                InExpr{
                    .expr = box(std::move(expr)),
                    .match = box(std::move(match)),
                    .match_field_id = match_field_id,
                },
            .value_type = ValueType::Boolean,
            .level = ExprKindLevel::Row,
            .required_fields = req_fields,
        };
    }

    Expr bindExpr(ast::LikeExpr e) {
        auto arg = bindExpr(std::move(*e.expr));
        require(arg.value_type == ValueType::String, "LIKE argument should be String");
        auto level = arg.level;
        auto req_fields = arg.required_fields;

        return {
            .node =
                LikeExpr{
                    .expr = box(std::move(arg)),
                    .regex = std::move(e.regex),
                },
            .value_type = ValueType::Boolean,
            .level = level,
            .required_fields = req_fields,
        };
    }

    Expr bindExpr(ast::FnCallExpr e) {
        std::vector<Expr> args;
        args.reserve(e.args.size());
        for (auto&& arg : e.args) {
            args.push_back(bindExpr(std::move(arg)));
        }

        auto fields = requiredFieldsOf(args);
        auto level = ExprKindLevel::Const;
        for (auto&& arg : args) {
            require(
                composable(level, arg.level),
                "different expression kinds not allowed in function calls");
            level = composed(level, arg.level);
        }

        if (auto un_aggr_type = unaryAggregateExprType(e.func)) {
            require(args.size() == 1, "function expects 1 argument");
            require(
                args[0].level != ExprKindLevel::Group,
                "grouping operations do not accept aggregates");
            auto value_type = unaryAggregateValueType(*un_aggr_type, args[0].value_type);

            return {
                .node =
                    UnaryAggregateExpr{
                        .type = *un_aggr_type,
                        .expr = box(std::move(args[0])),
                    },
                .value_type = value_type,
                .level = ExprKindLevel::Group,
                .required_fields = fields,
            };
        }

        if (e.func == "builtin_coalesce") {
            std::unordered_set<ValueType> types;
            for (auto&& arg : args) {
                types.insert(arg.value_type);
            }
            require(args.size() >= 1, "at least one argument required for COALESCE");
            require(types.size() == 1, "COALESCE arguments must have the same type");

            return {
                .node = CoalesceExpr{.args = std::move(args)},
                .value_type = *types.begin(),
                .level = level,
                .required_fields = fields,
            };
        }

        if (e.func == "builtin_percentile") {
            require(args.size() > 1, "PERCENTILE must be given at least one percentile");
            require(arithmetic(args[0].value_type), "PERCENTILE first argument must be arithmetic");
            require(args[0].level != ExprKindLevel::Group, "PERCENTILE does not accept aggregates");

            std::vector<float> percentiles;
            percentiles.reserve(args.size() - 1);
            for (size_t i = 1; i < args.size(); ++i) {
                require(
                    args[i].value_type == ValueType::Floating,
                    "PERCENTILE's arguments in positions >=1 must be floating");

                percentiles.push_back(
                    util::match(
                        std::move(args[i].node),
                        [](ValueExpr e) -> float { return e.value.get<float>(); },
                        [](auto) -> float {
                            throwError("PERCENTILE's arguments in positions >=1 must be literals");
                        }));
            }

            return {
                .node =
                    PercentileExpr{
                        .expr = box(std::move(args[0])),
                        .percentiles = std::move(percentiles),
                    },
                .value_type = ValueType::String,
                .level = ExprKindLevel::Group,
                .required_fields = fields,
            };
        }

        if (e.func == "builtin_rsubstr") {
            require(args.size() == 2, "RSUBSTR expects exactly 2 arguments");
            require(
                args[0].value_type == ValueType::String,
                "RSUBSTR's first argument should be String");
            require(
                args[1].level == ExprKindLevel::Const,
                "RSUBSTR's second argument should be String");
            require(
                args[1].value_type == ValueType::String,
                "RSUBSTR's second argument should be floating");

            std::string regex = util::match(
                args[1].node,
                [](ValueExpr e) { return e.value.get<std::string>(); },
                [](auto&&) -> std::string {
                    throwError("RSUBSTR's second argument should be literal");
                });

            return {
                .node =
                    RSubstrExpr{
                        .expr = box(std::move(args[0])),
                        .regex = std::move(regex),
                    },
                .value_type = ValueType::String,
                .level = level,
                .required_fields = fields,
            };
        }

        throwError("unknown function name: {}", e.func);
    }

    Expr bindExpr(ast::BinaryExpr e) {
        auto type = exprType(e.type);
        auto left = bindExpr(std::move(*e.left));
        auto right = bindExpr(std::move(*e.right));
        auto value_type = valueType(left.value_type, right.value_type, type);

        require(
            composable(left.level, right.level),
            "Binary operations require same expression level on both sides");

        auto level = composed(left.level, right.level);
        auto req_fields = FieldSet::merge(left.required_fields, right.required_fields);

        return {
            .node =
                BinaryExpr{
                    .type = type,
                    .left = box(std::move(left)),
                    .right = box(std::move(right)),
                },
            .value_type = value_type,
            .level = level,
            .required_fields = req_fields,
        };
    }

    Expr bindExpr(ast::UnaryExpr e) {
        auto arg = bindExpr(std::move(*e.expr));
        auto type = exprType(e.type);
        auto value_type = valueType(arg.value_type, type);
        auto level = arg.level;
        auto req_fields = arg.required_fields;

        return {
            .node =
                UnaryExpr{
                    .type = type,
                    .expr = box(std::move(arg)),
                },
            .value_type = value_type,
            .level = level,
            .required_fields = req_fields,
        };
    }

    void bindProjector(ast::Projector p, std::vector<Projector>& out) {
        util::match(
            std::move(p),
            [&](ast::StarProjector) { out.emplace_back(StarProjector{}); },
            [&](ast::IdentifierProjector p) {
                auto type = currFieldSet().typeOfSourceField(p.identifier, binding_);
                auto id = binding_->getOrAdd(p.identifier, type);
                out.emplace_back(IdentifierProjector{.field_id = id});
            },
            [&](ast::ExprProjector p) {
                auto expr = bindExpr(std::move(*p.expr));

                out.emplace_back(
                    ExprProjector{
                        .alias_field_id = binding_->getOrAdd(p.alias, expr.value_type),
                        .expr = box(std::move(expr)),
                    });
            });
    }

    std::vector<Projector> bindProjectors(std::vector<ast::Projector> projectors) {
        std::vector<Projector> result;
        result.reserve(projectors.size());
        for (auto&& p : projectors) {
            bindProjector(std::move(p), result);
        }
        return result;
    }

    std::vector<Expr> bindExprs(std::vector<ast::Expr> exprs) {
        std::vector<Expr> result;
        result.reserve(exprs.size());
        for (auto&& p : exprs) {
            result.push_back(bindExpr(std::move(p)));
        }
        return result;
    }

    FieldSet requiredFieldsOf(const Projector& p) {
        return util::match(
            p,
            [](const StarProjector&) { return FieldSet::emptySet(); },
            [](const IdentifierProjector& p) { return FieldSet::withField(p.field_id); },
            [](const ExprProjector& p) { return p.expr->required_fields; });
    }

    FieldSet requiredFieldsOf(const std::vector<Projector>& ps) {
        auto fields = FieldSet::emptySet();
        for (auto&& p : ps) {
            fields.merge(requiredFieldsOf(p));
        }
        return fields;
    }

    FieldSet requiredFieldsOf(const std::vector<Expr>& ps) {
        auto fields = FieldSet::emptySet();
        for (auto&& p : ps) {
            fields.merge(p.required_fields);
        }
        return fields;
    }

    FieldSet outputFieldsOf(const Projector& p) {
        return util::match(
            p,
            [](const StarProjector&) { return FieldSet::emptySet(); },
            [](const IdentifierProjector& p) { return FieldSet::withField(p.field_id); },
            [](const ExprProjector& p) { return FieldSet::withField(p.alias_field_id); });
    }

    FieldSet outputFieldsOf(const std::vector<Projector>& ps) {
        auto fields = FieldSet::emptySet();
        for (auto&& p : ps) {
            fields.merge(outputFieldsOf(p));
        }
        return fields;
    }

    std::unordered_map<FieldId, Projector*> buildMap(std::vector<Projector>& ps) {
        std::unordered_map<FieldId, Projector*> map;
        for (auto&& p : ps) {
            util::matchPartial(
                p,
                [&](IdentifierProjector& pp) { map.emplace(pp.field_id, &p); },
                [&](ExprProjector& pp) { map.emplace(pp.alias_field_id, &p); });
        }
        return map;
    }

    struct FieldSetChain {
        FieldSetChain(FieldSetNode* top, FieldSetChain* parent) : current_(top), parent_(parent) {}

        ValueType typeOfSourceField(std::string_view name, FieldBindingPtr binding) {
            if (current_ == nullptr) {
                throwError("unknown field {}", name);
            }

            for (auto id : current_->fieldSet().fieldIds()) {
                if (binding->name(id) == name) {
                    return binding->type(id);
                }
            }

            if (current_->hasUnknown()) {
                // Some sources do not know which fields they contain.
                // They are modeled like they contain fields of any name with String type.
                current_->addUnknown(binding->getOrAdd(name, ValueType::String));
                return ValueType::String;
            }

            if (parent_ == nullptr) {
                throwError("unknown field: {}", name);
            }
            return parent_->typeOfSourceField(name, binding);
        }

     private:
        FieldSetNode* current_;
        FieldSetChain* parent_;
    };

    struct ScopedFieldSet : util::Pinned {
        FieldSetChain** slot;
        FieldSetChain* old;
        ~ScopedFieldSet() { *slot = old; }
    };

    ScopedFieldSet scopedFieldSet(FieldSetChain* curr) {
        return ScopedFieldSet{
            .slot = &curr_field_set_slot_,
            .old = std::exchange(curr_field_set_slot_, curr),
        };
    }

    FieldSetChain& currFieldSet() {
        verify(curr_field_set_slot_);
        return *curr_field_set_slot_;
    }

    Program program_;
    FieldBindingPtr binding_;
    std::unordered_map<std::string, Relation*> named_relations_;
    FieldSetChain* curr_field_set_slot_ = nullptr;
};

}  // namespace

Program bind(ast::Program program) {
    return Binder().bind(std::move(program));
}

}  // namespace lsql::iface::sql::bind
