#pragma once

#include "iface/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/ast/Relations.h"    // IWYU pragma: keep
#include "iface/sql/ast/Statement.h"    // IWYU pragma: keep

#include <magic_enum/magic_enum.hpp>

#include <format>
#include <sstream>

namespace lsql::iface::sql::ast {

class Stringifier {
 public:
    Stringifier() = default;

    std::string print(const Program& program) {
        std::stringstream ss;

        for (auto&& s : program) {
            ss << print(s) << '\n';
        }

        return ss.str();
    }

 private:
    struct IndentScope {
        IndentScope(int* indent) : indent(indent) { ++*indent; }
        ~IndentScope() { --*indent; }
        int* indent;
    };

    auto indentScope() { return IndentScope{&indent_}; }

    std::string indent() const {
        std::stringstream ss;
        for (int i = 0; i < indent_; ++i) {
            ss << "  ";
        }
        return ss.str();
    }
    std::string print(const Statement& st) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, st);
    }

    std::string print(const QueryStatement& s) {
        std::string relation;
        {
            auto _ = indentScope();
            relation = print(*s.relation);
        }
        return std::format("{}QueryStatement:\n{}", indent(), relation);
    }

    std::string print(const NamedRelationStatement& s) {
        std::string relation;
        {
            auto _ = indentScope();
            relation = print(*s.relation);
        }
        return std::format("{}Name to {}:\n{}", indent(), s.name, relation);
    }

    std::string print(const Relation& r) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, r);
    }

    std::string print(const AdhocRelation& r) {
        std::stringstream ss;
        ss << indent() << "Adhoc (";
        for (auto&& literal : r.literals) {
            ss << std::format(
                "[{} (type {})],", literal.value_str, magic_enum::enum_name(literal.type));
        }
        ss << ")";
        return ss.str();
    }

    std::string print(const SelectRelation& r) {
        std::stringstream ss;
        ss << indent() << "SelectRelation source:";
        {
            auto _ = indentScope();
            ss << '\n' << print(*r.source);
        }
        ss << '\n' << indent() << "Projectors:";
        for (auto&& proj : r.projectors) {
            auto _ = indentScope();
            ss << '\n' << print(proj);
        }
        if (r.where) {
            ss << '\n' << indent() << "Where:";
            {
                auto _ = indentScope();
                ss << '\n' << print(*r.where);
            }
        }
        if (r.limit) {
            ss << '\n' << indent() << "Limit:";
            {
                auto _ = indentScope();
                ss << '\n' << print(*r.limit);
            }
        }
        if (r.order_by) {
            ss << '\n' << indent() << "OrderBy:";
            {
                auto _ = indentScope();
                ss << '\n' << print(*r.order_by);
            }
        }
        if (r.group_by) {
            ss << '\n' << indent() << "GroupBy:";
            {
                auto _ = indentScope();
                ss << '\n' << print(*r.group_by);
            }
        }

        return ss.str();
    }

    std::string print(const Projector& p) {
        return std::visit([this](auto&& p) { return print(p); }, p);
    }

    std::string print(const StarProjector&) { return std::format("{}*-projector", indent()); }

    std::string print(const IdentifierProjector& p) {
        return std::format("{}Identifier projector '{}'", indent(), p.identifier);
    }

    std::string print(const ExprProjector& p) {
        std::stringstream ss;
        ss << indent() << std::format("Expression projector alias={}", p.alias);
        auto _ = indentScope();
        ss << '\n' << print(*p.expr);
        return ss.str();
    }

    std::string print(const Where& w) {
        std::stringstream ss;
        ss << indent() << "Where";
        auto _ = indentScope();
        ss << '\n' << print(*w.condition);
        return ss.str();
    }

    std::string print(const Limit& l) { return std::format("{}Limit={}", indent(), l.limit); }

    std::string print(const OrderBy& o) {
        std::stringstream ss;
        ss << indent() << std::format("OrderBy desc={}", o.desc);
        for (auto&& e : o.order_list) {
            auto _ = indentScope();
            ss << '\n' << print(e);
        }
        return ss.str();
    }

    std::string print(const GroupBy& g) {
        std::stringstream ss;
        ss << indent() << "GroupBy";
        for (auto&& p : g.group_list) {
            auto _ = indentScope();
            ss << '\n' << print(p);
        }
        return ss.str();
    }

    std::string print(const UnionAllRelation& r) {
        std::stringstream ss;
        ss << indent() << "UnionAll";
        auto _ = indentScope();
        ss << '\n' << print(*r.left);
        ss << '\n' << print(*r.right);
        return ss.str();
    }

    std::string print(const UnionAllSortedByRelation& r) {
        std::stringstream ss;
        ss << indent() << std::format("UnionAllSortedBy desc={}", r.order_by.desc);
        for (auto&& item : r.order_by.order_list) {
            auto _ = indentScope();
            ss << '\n' << print(item);
        }
        auto _ = indentScope();
        ss << '\n' << print(*r.left);
        ss << '\n' << print(*r.right);
        return ss.str();
    }

    std::string print(const FileRelation& r) { return std::format("{}File {}", indent(), r.path); }

    std::string print(const FileIntervalRelation& r) {
        return std::format(
            "{}File {} [from {}, interval {}s]", indent(), r.path, r.ts_from, r.interval_s);
    }

    std::string print(const NamedRelationReferenceRelation& r) {
        return std::format("{}Reference relation name={}", indent(), r.name);
    }

    std::string print(const MaterializeRelation& r) {
        std::stringstream ss;
        ss << indent() << "Materialize";
        auto _ = indentScope();
        ss << '\n' << print(*r.relation);
        return ss.str();
    }

    std::string print(const Expr& e) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, e);
    }

    std::string print(const IdentifierExpr& e) {
        return std::format("{}Identifier {}", indent(), e.identifier);
    }

    std::string print(const LiteralExpr& e) {
        return std::format(
            "{}Literal {} (type {})",
            indent(),
            e.literal.value_str,
            magic_enum::enum_name(e.literal.type));
    }

    std::string print(const CastExpr& e) {
        std::stringstream ss;
        ss << indent() << std::format("Cast to type {}", magic_enum::enum_name(e.cast_to));
        auto _ = indentScope();
        ss << '\n' << print(*e.expr);
        return ss.str();
    }

    std::string print(const InExpr& e) {
        std::stringstream ss;
        ss << indent() << "In (expr, source)";
        auto _ = indentScope();
        ss << '\n' << print(*e.expr);
        ss << '\n' << print(*e.source);
        return ss.str();
    }

    std::string print(const LikeExpr& e) {
        std::stringstream ss;
        ss << indent() << std::format("Like '{}'", e.regex);
        auto _ = indentScope();
        ss << '\n' << print(*e.expr);
        return ss.str();
    }

    std::string print(const FnCallExpr& e) {
        std::stringstream ss;
        ss << indent() << std::format("Function call name: {}", e.func);
        for (auto&& arg : e.args) {
            auto _ = indentScope();
            ss << '\n' << print(arg);
        }
        return ss.str();
    }

    std::string print(const BinaryExpr& e) {
        std::stringstream ss;
        ss << indent() << std::format("BinaryExpr type: {}", magic_enum::enum_name(e.type));
        auto _ = indentScope();
        ss << '\n' << print(*e.left);
        ss << '\n' << print(*e.right);
        return ss.str();
    }

    std::string print(const UnaryExpr& e) {
        std::stringstream ss;
        ss << indent() << std::format("UnaryExpr type: {}", magic_enum::enum_name(e.type));
        auto _ = indentScope();
        ss << '\n' << print(*e.expr);
        return ss.str();
    }
    int indent_ = 0;
};

}  // namespace lsql::iface::sql::ast
