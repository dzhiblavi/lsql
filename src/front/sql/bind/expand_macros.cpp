#include "front/sql/bind/expand_macros.h"

#include "front/sql/ast/Expressions.h"
#include "front/sql/ast/Relations.h"

#include "front/common/bind/Expressions.h"
#include "front/common/source/require_at.h"

#include "util/TreeWalker.h"
#include "util/deep_copy.h"

#include <unordered_map>
#include <unordered_set>

namespace lsql::front::sql::bind {

namespace {

struct ParameterSubstituter : util::TreeWalker<ParameterSubstituter> {
    using Base = util::TreeWalker<ParameterSubstituter>;
    using Base::visit;

    void visit(ast::Expr& expr) {
        if (auto* identifier = std::get_if<ast::IdentifierExpr>(&expr.node)) {
            if (auto it = arguments.find(identifier->identifier); it != arguments.end()) {
                expr = util::deepCopy(*it->second);
                return;
            }
        }
        Base::visit(expr);
    }

    void visit(ast::Projector& projector) {
        if (auto* identifier = std::get_if<ast::IdentifierProjector>(&projector.node)) {
            if (auto it = arguments.find(identifier->identifier); it != arguments.end()) {
                projector.node = ast::ExprProjector{
                    .alias = identifier->identifier,
                    .expr = box(util::deepCopy(*it->second)),
                };
                return;
            }
        }
        Base::visit(projector);
    }

    std::unordered_map<std::string, const ast::Expr*> arguments;
};

struct MacroBodyValidator : util::TreeWalker<MacroBodyValidator> {
    using Base = util::TreeWalker<MacroBodyValidator>;
    using Base::visit;

    void visit(ast::Expr& expr) {
        if (auto* identifier = std::get_if<ast::IdentifierExpr>(&expr.node)) {
            requireAt(
                relation_depth > 0 || parameters.contains(identifier->identifier),
                expr.span,
                "unknown macro parameter '{}'; pass fields as macro arguments",
                identifier->identifier);
        }
        Base::visit(expr);
    }

    void visit(ast::Relation& relation) {
        ++relation_depth;
        Base::visit(relation);
        --relation_depth;
    }

    std::unordered_set<std::string> parameters;
    unsigned relation_depth = 0;
};

struct MacroExpander : util::TreeWalker<MacroExpander> {
    using Base = util::TreeWalker<MacroExpander>;
    using Base::visit;

    void define(ast::MacroDefinitionStatement definition, SourceSpan span) {
        requireAt(
            !common::bind::isBuiltinFunction(definition.name) && definition.name != "count",
            span,
            "macro '{}' conflicts with a built-in function",
            definition.name);
        requireAt(
            !definitions.contains(definition.name), span, "duplicate macro '{}'", definition.name);

        MacroBodyValidator validator;
        for (const auto& parameter : definition.parameters) {
            requireAt(
                validator.parameters.insert(parameter.name).second,
                parameter.span,
                "duplicate macro parameter '{}'",
                parameter.name);
        }

        validator.visit(*definition.body);

        visit(*definition.body);

        definitions.emplace(definition.name, std::move(definition));
    }

    void visit(ast::Expr& expr) {
        Base::visit(expr);
        auto* call = std::get_if<ast::FnCallExpr>(&expr.node);
        if (!call) {
            return;
        }

        auto it = definitions.find(call->func);
        if (it == definitions.end()) {
            requireAt(
                common::bind::isBuiltinFunction(call->func),
                expr.span,
                "unknown function or macro '{}'; macros must be defined before use",
                call->func);
            return;
        }

        const auto& definition = it->second;
        requireAt(
            call->args.size() == definition.parameters.size(),
            expr.span,
            "macro '{}' expects {} arguments, got {}",
            call->func,
            definition.parameters.size(),
            call->args.size());

        ParameterSubstituter substituter;
        for (size_t index = 0; index < call->args.size(); ++index) {
            substituter.arguments.emplace(definition.parameters[index].name, &call->args[index]);
        }

        auto body = util::deepCopy(*definition.body);
        substituter.visit(body);
        body.span = expr.span;
        expr = std::move(body);
    }

    std::unordered_map<std::string, ast::MacroDefinitionStatement> definitions;
};

}  // namespace

ast::Program expandMacros(ast::Program program) {
    MacroExpander expander;
    ast::Program expanded;
    expanded.reserve(program.size());

    for (auto& statement : program) {
        if (auto* definition = std::get_if<ast::MacroDefinitionStatement>(&statement.node)) {
            expander.define(std::move(*definition), statement.span);
        } else {
            expander.visit(statement);
            expanded.push_back(std::move(statement));
        }
    }

    return expanded;
}

}  // namespace lsql::front::sql::bind
