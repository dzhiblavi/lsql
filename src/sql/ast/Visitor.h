#pragma once

namespace lsql::sql::ast {

class Program;
class NamedRelation;
class NamedRelationReference;
class FileReference;
class FileIntervalReference;
class SelectStatement;
class SelectItem;
class Limit;
class Where;
class GroupBySelect;
class OrderBy;
class OrderBySelect;

class BinaryExpression;
class UnaryExpression;
class IdentifierExpression;
class ValueExpression;
class CastExpression;
class UnaryAggregateExpression;
class PercentileExpression;
class InExpression;
class LikeExpression;
class RSubstrExpression;
class CoalesceExpression;

class Visitor {
 public:
    virtual ~Visitor() = default;
    virtual void visit(const Program&) {}
    virtual void visit(const NamedRelation&) {}
    virtual void visit(const NamedRelationReference&) {}
    virtual void visit(const FileReference&) {}
    virtual void visit(const FileIntervalReference&) {}
    virtual void visit(const SelectStatement&) {}
    virtual void visit(const GroupBySelect&) {}
    virtual void visit(const OrderBy&) {}
    virtual void visit(const OrderBySelect&) {}
    virtual void visit(const SelectItem&) {}
    virtual void visit(const Limit&) {}
    virtual void visit(const Where&) {}
    virtual void visit(const BinaryExpression&) {}
    virtual void visit(const UnaryExpression&) {}
    virtual void visit(const IdentifierExpression&) {}
    virtual void visit(const ValueExpression&) {}
    virtual void visit(const CastExpression&) {}
    virtual void visit(const UnaryAggregateExpression&) {}
    virtual void visit(const PercentileExpression&) {}
    virtual void visit(const InExpression&) {}
    virtual void visit(const LikeExpression&) {}
    virtual void visit(const RSubstrExpression&) {}
    virtual void visit(const CoalesceExpression&) {}
};

}  // namespace lsql::sql::ast
