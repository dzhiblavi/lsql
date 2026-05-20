#include "iface/sql/bind/Statement.h"

#include "iface/sql/ast/Expr.h"
#include "iface/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/ast/Relations.h"    // IWYU pragma: keep
#include "iface/sql/ast/Statement.h"    // IWYU pragma: keep

#include "iface/sql/bind/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/bind/Relations.h"    // IWYU pragma: keep

#include "iface/sql/bind/helpers.h"

#include "core/time_formats.h"

#include <magic_enum/magic_enum.hpp>

#include <algorithm>
#include <unordered_set>

namespace lsql::iface::sql::bind {

namespace {

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

UnaryAggregateExprType unaryAggregateType(std::string_view func_name) {
    static constexpr std::array<std::pair<std::string_view, UnaryAggregateExprType>, 4> Types{
        std::make_pair("builtin_count", UnaryAggregateExprType::Count),
        std::make_pair("builtin_min", UnaryAggregateExprType::Min),
        std::make_pair("builtin_max", UnaryAggregateExprType::Max),
        std::make_pair("builtin_sum", UnaryAggregateExprType::Sum),
    };

    auto it = std::ranges::find(Types, func_name, [](auto&& p) { return p.first; });
    verify(it != Types.end());
    return it->second;
}

ValueType valueType(UnaryAggregateExprType type, ValueType arg) {
    switch (type) {
        case UnaryAggregateExprType::Count:
            require(arg == ValueType::Boolean, "COUNT argument should be boolean");
            return ValueType::Integer;
        case UnaryAggregateExprType::Min:
            return arg;
        case UnaryAggregateExprType::Max:
            return arg;
        case UnaryAggregateExprType::Sum:
            require(arithmetic(arg), "SUM argument should be arithmetic");
            return arg;
    }
}

void add(const Projector& proj, RelationFields& out) {
    util::match(
        proj,
        [&](const StarProjector&) { out.setUnknown(); },
        [&](const ExprProjector& p) { out.add(p.alias, valueTypeOf(*p.expr)); });
}

void addAll(const std::vector<Projector>& projectors, RelationFields& out) {
    for (auto&& p : projectors) {
        add(p, out);
    }
}

class Binder {
 public:
    Binder() = default;

    Program bind(ast::Program program) && {
        program_.reserve(program.size());

        for (auto&& statement : program) {
            program_.push_back(bindStatement(std::move(statement)));
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
            .relation = std::make_unique<Relation>(bindRelation(std::move(*s.relation))),
        };
    }

    Statement bindStatement(ast::NamedRelationStatement s) {
        require(!named_relations_.contains(s.name), "duplicate named relation '{}'", s.name);

        auto relation = std::make_unique<Relation>(bindRelation(std::move(*s.relation)));
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

        auto fields = RelationFields::emptySet();
        auto type = values.empty() ? ValueType::Null : values.front().type();
        fields.add("anon1", type);

        return AdhocRelation{
            .values = std::move(values),
            .fields = std::move(fields),
        };
    }

    Relation bindRelation(ast::SelectRelation r) {
        auto projectors = bind(std::move(r.projectors));
        require(!projectors.empty(), "SELECT requires at least one projector");

        bool has_group_projector = false;
        bool has_row_projector = false;
        for (auto&& p : projectors) {
            util::match(
                p,
                [&](const StarProjector&) { has_row_projector = true; },
                [&](const ExprProjector& p) {
                    has_group_projector |= exprKindLevelOf(*p.expr) == ExprKindLevel::Group;
                    has_row_projector |= exprKindLevelOf(*p.expr) == ExprKindLevel::Row;
                });
        }

        auto source = bindRelation(std::move(*r.source));
        auto fields = RelationFields::emptySet();
        addAll(projectors, fields);

        std::optional<Limit> limit;
        if (r.limit) {
            limit = Limit{.limit = r.limit->limit};
        }
        std::optional<Where> where;
        if (r.where) {
            auto cond = bindExpr(std::move(*r.where->condition));
            require(valueTypeOf(cond) == ValueType::Boolean, "WHERE condition must be boolean");
            require(
                exprKindLevelOf(cond) != ExprKindLevel::Group,
                "WHERE condition cannot be aggregate");

            where = Where{.condition = std::make_unique<Expr>(std::move(cond))};
        }
        std::optional<OrderBy> order_by;
        if (r.order_by) {
            order_by = OrderBy{
                .order_list = bind(std::move(r.order_by->order_list)),
                .desc = r.order_by->desc,
            };
        }
        std::optional<GroupBy> group_by;
        if (r.group_by) {
            auto group_by_list = bind(std::move(r.group_by->group_list));

            for (auto&& p : group_by_list) {
                util::match(
                    p,
                    [](const StarProjector&) {
                        throw std::runtime_error("* is not allowed in group expression");
                    },
                    [&](const ExprProjector& p) {
                        require(
                            exprKindLevelOf(*p.expr) != ExprKindLevel::Group,
                            "cannot aggregate aggregates");
                    });
            }

            group_by = GroupBy{
                .group_list = std::move(group_by_list),
            };
        } else {
            require(
                !(has_group_projector && has_row_projector), "cannot mix group and row projectors");
        }

        return SelectRelation{
            .projectors = std::move(projectors),
            .source = std::make_unique<Relation>(std::move(source)),
            .fields = std::move(fields),
            .aggregate = has_group_projector,
            .limit = std::move(limit),
            .where = std::move(where),
            .order_by = std::move(order_by),
            .group_by = std::move(group_by),
        };
    }

    Relation bindRelation(ast::UnionAllRelation r) {
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));
        auto fields = RelationFields::merge(fieldsOf(left), fieldsOf(right));

        return UnionAllRelation{
            .left = std::make_unique<Relation>(std::move(left)),
            .right = std::make_unique<Relation>(std::move(right)),
            .fields = std::move(fields),
        };
    }

    Relation bindRelation(ast::UnionAllSortedByRelation r) {
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));
        auto fields = RelationFields::merge(fieldsOf(left), fieldsOf(right));

        std::vector<Expr> order_list;
        order_list.reserve(r.order_by.order_list.size());
        for (auto&& expr : r.order_by.order_list) {
            order_list.push_back(bindExpr(std::move(expr)));
        }
        require(!order_list.empty(), "order list cannot be empty");

        return UnionAllSortedByRelation{
            .left = std::make_unique<Relation>(std::move(left)),
            .right = std::make_unique<Relation>(std::move(right)),
            .order_by =
                OrderBy{
                    .order_list = std::move(order_list),
                    .desc = r.order_by.desc,
                },
            .fields = std::move(fields),
        };
    }

    Relation bindRelation(ast::FileRelation r) {
        return FileRelation{
            .path = std::move(r.path),
            .fields = RelationFields::unknownSet(),
        };
    }

    Relation bindRelation(ast::FileIntervalRelation r) {
        constexpr auto TimeFormat = TimeFormat::ISO8601;
        auto ts_from = timestampFromString(r.ts_from, TimeFormat);

        return FileIntervalRelation{
            .path = std::move(r.path),
            .ts_from = ts_from,
            .ts_to = ts_from + r.interval_s,
            .fields = RelationFields::unknownSet(),
        };
    }

    Relation bindRelation(ast::NamedRelationReferenceRelation r) {
        auto it = named_relations_.find(r.name);
        require(it != named_relations_.end(), "no named relation '{}'", r.name);

        return NamedRelationReferenceRelation{
            .name = std::move(r.name),
            .fields = fieldsOf(*it->second),
        };
    }

    Relation bindRelation(ast::MaterializeRelation r) {
        auto arg = bindRelation(std::move(*r.relation));
        auto fields = fieldsOf(arg);

        return MaterializeRelation{
            .relation = std::make_unique<Relation>(std::move(arg)),
            .fields = std::move(fields),
        };
    }

    Expr bindExpr(ast::IdentifierExpr e) {
        return FieldExpr{
            .identifier = std::move(e.identifier),
        };
    }

    Expr bindExpr(ast::LiteralExpr e) {
        return ValueExpr{
            .value = parseLiteral(e.literal),
        };
    }

    Expr bindExpr(ast::CastExpr e) {
        return CastExpr{
            .cast_to = e.cast_to,
            .expr = std::make_unique<Expr>(bindExpr(std::move(*e.expr))),
        };
    }

    Expr bindExpr(ast::InExpr e) {
        return InExpr{
            .expr = std::make_unique<Expr>(bindExpr(std::move(*e.expr))),
            .source = std::make_unique<Relation>(bindRelation(std::move(*e.source))),
        };
    }

    Expr bindExpr(ast::LikeExpr e) {
        auto arg = bindExpr(std::move(*e.expr));
        require(valueTypeOf(arg) == ValueType::String, "LIKE argument should be String");

        return LikeExpr{
            .expr = std::make_unique<Expr>(std::move(arg)),
            .regex = std::move(e.regex),
        };
    }

    Expr bindExpr(ast::FnCallExpr e) {
        std::vector<Expr> args;
        args.reserve(e.args.size());
        for (auto&& arg : e.args) {
            args.push_back(bindExpr(std::move(arg)));
        }

        if (isUnaryGroupFunc(e.func)) {
            require(args.size() == 1, "function expects 1 argument");
            require(
                std::ranges::all_of(
                    args, [](auto&& a) { return exprKindLevelOf(a) != ExprKindLevel::Group; }),
                "grouping operations do not accept aggregates");

            auto type = unaryAggregateType(e.func);
            auto value_type = valueType(type, valueTypeOf(args[0]));

            return UnaryAggregateExpr{
                .type = type,
                .value_type = value_type,
                .expr = std::make_unique<Expr>(std::move(args[0])),
            };
        }

        if (e.func == "builtin_coalesce") {
            require(args.size() >= 1, "at least one argument required for COALESCE");
            ExprKindLevel level = ExprKindLevel::Const;
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

            return CoalesceExpr{
                .args = std::move(args),
            };
        }

        if (e.func == "builtin_percentile") {
            require(args.size() > 1, "PERCENTILE must be given at least one percentile");
            require(
                arithmetic(valueTypeOf(args[0])), "PERCENTILE first argument must be arithmetic");
            require(
                exprKindLevelOf(args[0]) != ExprKindLevel::Group,
                "PERCENTILE does not accept aggregates");

            std::vector<float> percentiles;
            percentiles.reserve(args.size() - 1);
            for (size_t i = 1; i < args.size(); ++i) {
                percentiles.push_back(
                    util::match(
                        args[i],
                        [](ValueExpr e) {
                            require(
                                e.value.type() == ValueType::Floating,
                                "PERCENTILE's arguments in positions >=1 must be floating");
                            float p = e.value.get<float>();
                            require(0.f <= p && p <= 1.f, "percentiles must be in [0.0, 1.0]");
                            return p;
                        },
                        [](auto&&) -> float {
                            throw std::runtime_error(
                                "PERCENTILE's arguments in positions >=1 must be literals");
                        }));
            }

            return PercentileExpr{
                .expr = std::make_unique<Expr>(std::move(args[0])),
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
                [](ValueExpr e) {
                    require(
                        e.value.type() == ValueType::String,
                        "RSUBSTR's second argument should be String, got {}",
                        magic_enum::enum_name(e.value.type()));
                    return e.value.get<std::string>();
                },
                [](auto&&) -> std::string {
                    throw std::runtime_error("RSUBSTR's second argument should be a literal");
                });

            return RSubstrExpr{
                .expr = std::make_unique<Expr>(std::move(args[0])),
                .regex = std::move(regex),
            };
        }

        throw std::runtime_error(std::format("unknown function name {}", e.func));
    }

    Expr bindExpr(ast::BinaryExpr e) {
        auto type = exprType(e.type);
        auto left = bindExpr(std::move(*e.left));
        auto right = bindExpr(std::move(*e.right));
        auto value_type = valueType(valueTypeOf(left), valueTypeOf(right), type);

        require(
            composable(exprKindLevelOf(left), exprKindLevelOf(right)),
            "Binary operations require same expression level on both sides");

        return BinaryExpr{
            .type = type,
            .value_type = value_type,
            .left = std::make_unique<Expr>(std::move(left)),
            .right = std::make_unique<Expr>(std::move(right)),
        };
    }

    Expr bindExpr(ast::UnaryExpr e) {
        auto arg = bindExpr(std::move(*e.expr));
        auto type = exprType(e.type);
        auto value_type = valueType(valueTypeOf(arg), type);

        return UnaryExpr{
            .type = type,
            .value_type = value_type,
            .expr = std::make_unique<Expr>(std::move(arg)),
        };
    }

    Projector bind(ast::Projector p) {
        return util::match(
            std::move(p),
            [](ast::StarProjector) -> Projector { return StarProjector{}; },
            [](ast::IdentifierProjector p) -> Projector {
                return ExprProjector{
                    .alias = p.identifier,
                    .expr = std::make_unique<Expr>(FieldExpr{
                        .identifier = p.identifier,
                    }),
                };
            },
            [this](ast::ExprProjector p) -> Projector {
                return ExprProjector{
                    .alias = std::move(p.alias),
                    .expr = std::make_unique<Expr>(bindExpr(std::move(*p.expr))),
                };
            });
    }

    std::vector<Projector> bind(std::vector<ast::Projector> projectors) {
        std::vector<Projector> result;
        result.reserve(projectors.size());
        for (auto&& p : projectors) {
            result.push_back(bind(std::move(p)));
        }
        return result;
    }

    std::vector<Expr> bind(std::vector<ast::Expr> exprs) {
        std::vector<Expr> result;
        result.reserve(exprs.size());
        for (auto&& p : exprs) {
            result.push_back(bindExpr(std::move(p)));
        }
        return result;
    }

    Program program_;
    std::unordered_map<std::string, const Relation*> named_relations_;
};

}  // namespace

Program bind(ast::Program program) {
    return Binder().bind(std::move(program));
}

}  // namespace lsql::iface::sql::bind
