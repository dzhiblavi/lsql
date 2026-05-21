#pragma once

#include "iface/sql/ast/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/ast/Relations.h"    // IWYU pragma: keep
#include "iface/sql/ast/Statement.h"    // IWYU pragma: keep

#include "util/StrBuilder.h"

#include <magic_enum/magic_enum.hpp>

#include <format>
#include <sstream>

namespace lsql::iface::sql::ast {

inline std::string to_string(const ast::Literal& v) {
    return std::format("str={},type={}", v.value_str, magic_enum::enum_name(v.type));
}

class Stringifier {
    using StrBuilder = util::StrBuilder;

 public:
    Stringifier() = default;

    std::string print(const Program& program) {
        auto b = StrBuilder("Program AST");
        for (auto&& s : program) {
            b.child(print(s));
        }
        return b.render();
    }

 private:
    template <typename T>
    static std::string toString(const std::vector<T>& values) {
        using std::to_string;

        std::stringstream ss;
        ss << '[';
        for (auto&& v : values) {
            ss << to_string(v) << ',';
        }
        if (!values.empty()) {
            ss.seekp(-1, std::ios_base::end);
        }
        ss << ']';
        return ss.str();
    }

    StrBuilder print(const Statement& st) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, st);
    }

    StrBuilder print(const QueryStatement& s) {
        return StrBuilder("QueryStatement").child(print(*s.relation));
    }

    StrBuilder print(const NamedRelationStatement& s) {
        return StrBuilder("NamedRelationStatement name={}", s.name).child(print(*s.relation));
    }

    StrBuilder print(const Relation& r) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, r);
    }

    StrBuilder print(const AdhocRelation& r) {
        return StrBuilder("AdhocRelation count={}", r.literals.size())
            .child(StrBuilder("literals").child(toString(r.literals)));
    }

    StrBuilder print(const SelectRelation& r) {
        auto p = StrBuilder("projectors");
        for (auto&& proj : r.projectors) {
            p.child(print(proj));
        }

        auto b = StrBuilder("SelectRelation")
                     .child(StrBuilder("source").child(print(*r.source)))
                     .child(p);

        if (r.where) {
            b.child(print(*r.where));
        }
        if (r.limit) {
            b.child(print(*r.limit));
        }
        if (r.order_by) {
            b.child(print(*r.order_by));
        }
        if (r.group_by) {
            b.child(print(*r.group_by));
        }

        return b;
    }

    StrBuilder print(const Projector& p) {
        return std::visit([this](auto&& p) { return print(p); }, p);
    }

    StrBuilder print(const StarProjector&) { return StrBuilder("*-projector"); }

    StrBuilder print(const IdentifierProjector& p) {
        return StrBuilder("IdentifierProjector '{}'", p.identifier);
    }

    StrBuilder print(const ExprProjector& p) {
        return StrBuilder("ExprProjector alias={}", p.alias).child(print(*p.expr));
    }

    StrBuilder print(const Where& w) { return StrBuilder("Where").child(print(*w.condition)); }

    StrBuilder print(const Limit& l) { return StrBuilder("Limit={}", l.limit); }

    StrBuilder print(const OrderBy& o) {
        auto b = StrBuilder("OrderBy desc={}", o.desc);
        for (auto&& e : o.order_list) {
            b.child(StrBuilder("- child").child(print(e)));
        }
        return b;
    }

    StrBuilder print(const GroupBy& g) {
        auto b = StrBuilder("GroupBy");
        for (auto&& p : g.group_list) {
            b.child(print(p));
        }
        return b;
    }

    StrBuilder print(const UnionAllRelation& r) {
        return StrBuilder("UnionAll")
            .child(StrBuilder("- left").child(print(*r.left)))
            .child(StrBuilder("- right").child(print(*r.right)));
    }

    StrBuilder print(const UnionAllSortedByRelation& r) {
        auto order = StrBuilder("order list");
        for (auto&& item : r.order_by.order_list) {
            order.child(print(item));
        }

        return StrBuilder("UnionAll")
            .child(StrBuilder("- left").child(print(*r.left)))
            .child(StrBuilder("- right").child(print(*r.right)))
            .child(order);
    }

    StrBuilder print(const FileRelation& r) { return StrBuilder("FileRelation path={}", r.path); }

    StrBuilder print(const FileIntervalRelation& r) {
        return StrBuilder(
            "FileIntervalRelation path={} ts_from={}, interval={}",
            r.path,
            r.ts_from,
            r.interval_s);
    }

    StrBuilder print(const NamedRelationReferenceRelation& r) {
        return StrBuilder("NamedRelationReferenceRelation name={}", r.name);
    }

    StrBuilder print(const MaterializeRelation& r) {
        return StrBuilder("MaterializeRelation").child(print(*r.relation));
    }

    StrBuilder print(const Expr& e) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, e);
    }

    StrBuilder print(const IdentifierExpr& e) {
        return StrBuilder("IdentifierExpr id={}", e.identifier);
    }

    StrBuilder print(const LiteralExpr& e) {
        return StrBuilder("LiteralExpr literal={}", to_string(e.literal));
    }

    StrBuilder print(const CastExpr& e) {
        return StrBuilder("CastExpr to type {}", magic_enum::enum_name(e.cast_to))
            .child(print(*e.expr));
    }

    StrBuilder print(const InExpr& e) {
        return StrBuilder("InExpr")
            .child(StrBuilder("expression").child(print(*e.expr)))
            .child(StrBuilder("match set").child(print(*e.match)));
    }

    StrBuilder print(const LikeExpr& e) {
        return StrBuilder("LikeExpr '{}'", e.regex).child(print(*e.expr));
    }

    StrBuilder print(const FnCallExpr& e) {
        auto a = StrBuilder("args");
        for (auto&& arg : e.args) {
            a.child(print(arg));
        }
        return StrBuilder("FnCallExpr name={}", e.func).child(a);
    }

    StrBuilder print(const BinaryExpr& e) {
        return StrBuilder("BinaryExpr type: {}", magic_enum::enum_name(e.type))
            .child(StrBuilder("left").child(print(*e.left)))
            .child(StrBuilder("right").child(print(*e.right)));
    }

    StrBuilder print(const UnaryExpr& e) {
        return StrBuilder("UnaryExpr type: {}", magic_enum::enum_name(e.type))
            .child(print(*e.expr));
    }
};

}  // namespace lsql::iface::sql::ast
