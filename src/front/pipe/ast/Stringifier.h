#pragma once

#include "front/pipe/ast/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/ast/Pipeline.h"
#include "front/pipe/ast/Sources.h"
#include "front/pipe/ast/Stages.h"

#include "front/pipe/ast/Statements.h"
#include "util/StrBuilder.h"
#include "util/string.h"

#include <magic_enum/magic_enum.hpp>

#include <format>

namespace lsql::front::pipe::ast {

inline std::string to_string(const ast::Literal& v) {
    return std::format("{}({})", magic_enum::enum_name(v.type), v.value_str);
}

class Stringifier {
    using StrBuilder = util::StrBuilder;

 public:
    Stringifier() = default;

    std::string print(const Program& p) {
        auto b = StrBuilder("Program");
        for (auto&& st : p.statements) {
            b.item(print(st));
        }
        return b.render();
    }

    StrBuilder print(const Pipeline& p) {
        auto b = print(*p.source);
        for (auto&& s : p.stages) {
            b.item(print(s));
        }
        return b;
    }

 private:
    StrBuilder print(const Statement& s) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, s);
    }

    StrBuilder print(const QueryStatement& s) {
        return StrBuilder("Query").child(print(*s.pipeline));
    }

    StrBuilder print(const NamedPipelineStatement& s) {
        return StrBuilder("NamedPipeline name={}", s.name).child(print(*s.pipeline));
    }

    StrBuilder print(const Source& s) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, s);
    }

    StrBuilder print(const AdhocSource& r) {
        return StrBuilder("Adhoc count={}", r.literals.size())
            .child(StrBuilder("literals").child(util::toString(r.literals)));
    }

    StrBuilder print(const NamedPipelineReferenceSource& r) {
        return StrBuilder("NamedPipelineReference name: {}", r.name);
    }

    StrBuilder print(const FileSource& r) { return StrBuilder("FileSource path={}", r.path); }

    StrBuilder print(const FileIntervalSource& r) {
        return StrBuilder(
            "FileIntervalSource path={} ts_from={}, interval={}", r.path, r.ts_from, r.interval_s);
    }

    StrBuilder print(const UnionAllSource& r) {
        return StrBuilder("UnionAll")
            .item(StrBuilder("left").child(print(*r.left)))
            .item(StrBuilder("right").child(print(*r.right)));
    }

    StrBuilder print(const UnionAllSortedBySource& r) {
        auto order = StrBuilder("order list");
        for (auto&& item : r.order_list) {
            order.item(print(item));
        }

        return StrBuilder("UnionAllSortedBy desc={}", r.desc)
            .item(StrBuilder("left").child(print(*r.left)))
            .item(StrBuilder("right").child(print(*r.right)))
            .child(order);
    }

    StrBuilder print(const Stage& st) {
        return std::visit([this](auto&& arg) { return this->print(arg); }, st);
    }

    StrBuilder print(const FilterStage& f) {
        return StrBuilder("Filter").child(print(*f.condition));
    }

    StrBuilder print(const TakeStage& t) { return StrBuilder("Take={}", t.count); }

    StrBuilder print(const SortStage& s) {
        auto b = StrBuilder("OrderBy desc={}", s.desc);
        for (auto&& e : s.order_list) {
            b.item(print(e));
        }
        return b;
    }

    StrBuilder print(const GroupStage& s) {
        auto p = StrBuilder("projectors");
        for (auto&& proj : s.projectors) {
            p.item(print(proj));
        }
        auto g = StrBuilder("group key");
        for (auto&& proj : s.group_list) {
            g.item(print(proj));
        }
        return StrBuilder("Group").child(p).child(g);
    }

    StrBuilder print(const WhereInStage& s) {
        return StrBuilder("WhereIn")
            .child(StrBuilder("expr").block(print(*s.expr)))
            .child(StrBuilder("match").block(print(*s.match)));
    }

    StrBuilder print(const SelectStage& r) {
        auto p = StrBuilder("projectors");
        for (auto&& proj : r.projectors) {
            p.item(print(proj));
        }
        return StrBuilder("Select").child(p);
    }

    StrBuilder print(const Projector& p) {
        return std::visit([this](auto&& p) { return print(p); }, p);
    }

    StrBuilder print(const StarProjector& /*p*/) { return StrBuilder("StarProjector"); }

    StrBuilder print(const IdentifierProjector& p) {
        return StrBuilder("IdentifierProjector '{}'", p.identifier);
    }

    StrBuilder print(const ExprProjector& p) {
        return StrBuilder("ExprProjector alias={}", p.alias).child(print(*p.expr));
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

    StrBuilder print(const LikeExpr& e) {
        return StrBuilder("LikeExpr '{}'", e.regex).child(print(*e.expr));
    }

    StrBuilder print(const InExpr& e) {
        return StrBuilder("InExpr")
            .child(StrBuilder("expr").block(print(*e.expr)))
            .child(StrBuilder("match").block(print(*e.match)));
    }

    StrBuilder print(const FnCallExpr& e) {
        auto a = StrBuilder("args");
        for (auto&& arg : e.args) {
            a.item(print(arg));
        }
        return StrBuilder("FnCallExpr name={}", e.func).child(a);
    }

    StrBuilder print(const BinaryExpr& e) {
        return StrBuilder("BinaryExpr type: {}", magic_enum::enum_name(e.type))
            .item(StrBuilder("left").child(print(*e.left)))
            .item(StrBuilder("right").child(print(*e.right)));
    }

    StrBuilder print(const UnaryExpr& e) {
        return StrBuilder("UnaryExpr type: {}", magic_enum::enum_name(e.type))
            .child(print(*e.expr));
    }
};

}  // namespace lsql::front::pipe::ast
