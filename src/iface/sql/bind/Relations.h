#pragma once

#include "iface/sql/bind/Expr.h"
#include "iface/sql/bind/Relation.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/types.h"

#include <optional>
#include <vector>

namespace lsql::iface::sql::bind {

class FieldSetNode;
using FieldSetNodePtr = Arc<FieldSetNode>;

// This thing represents a vertial FieldSet inclusion
class FieldSetNode {
 public:
    static FieldSetNodePtr unknownSet();
    static FieldSetNodePtr emptySet();

    template <std::same_as<FieldSetNodePtr>... Children>
    static FieldSetNodePtr make(FieldSet set, Children... children);

    template <std::same_as<FieldSetNodePtr>... Children>
    static FieldSetNodePtr proxy(Children... children);

    FieldSetNode(FieldSetNode&& rhs) noexcept
        : current_(std::move(rhs.current_))
        , children_(std::move(rhs.children_))
        , self_unknown_(rhs.self_unknown_)
        , has_unknown_(rhs.has_unknown_) {}

    const FieldSet& fieldSet() const { return current_; }

    FieldSet subtreeFieldSet() const {
        auto set = current_;
        for (auto child : children_) {
            set.merge(child->subtreeFieldSet());
        }
        return set;
    }

    void merge(const FieldSet& b) {
        verify(!hasUnknown());
        verify(children_.empty());
        current_.merge(b);
    }

    bool hasUnknown() const { return has_unknown_; }

    void addUnknown(FieldId id) {
        if (self_unknown_) {
            current_.add(id);
        }
        for (auto child : children_) {
            child->addUnknown(id);
        }
    }

    template <std::same_as<FieldSetNodePtr>... Children>
    FieldSetNode(bool self_unknown, bool inherit_now, FieldSet set, Children... children)
        : current_(std::move(set))
        , children_{children...}
        , self_unknown_(self_unknown)
        , has_unknown_(self_unknown) {
        has_unknown_ |= (children->hasUnknown() || ...);

        if (self_unknown_) {
            verify(children_.empty());
        }

        if (inherit_now) {
            verify(!self_unknown_);
            for (auto child : children_) {
                current_.merge(child->fieldSet());
            }
        }
    }

 private:
    FieldSet current_;
    const std::vector<FieldSetNodePtr> children_;
    const bool self_unknown_;
    bool has_unknown_;
};

inline FieldSetNodePtr FieldSetNode::unknownSet() {
    return arc<FieldSetNode>(true, false, FieldSet::emptySet());
}

inline FieldSetNodePtr FieldSetNode::emptySet() {
    return arc<FieldSetNode>(false, false, FieldSet::emptySet());
}

template <std::same_as<FieldSetNodePtr>... Children>
FieldSetNodePtr FieldSetNode::make(FieldSet set, Children... children) {
    return arc<FieldSetNode>(false, false, std::move(set), children...);
}

// fields are same as chilren's
template <std::same_as<FieldSetNodePtr>... Children>
FieldSetNodePtr FieldSetNode::proxy(Children... children) {
    return arc<FieldSetNode>(false, true, FieldSet::emptySet(), children...);
}

struct StarProjector;
struct IdentifierProjector;
struct ExprProjector;

using Projector = std::variant<StarProjector, IdentifierProjector, ExprProjector>;

struct StarProjector {};

struct IdentifierProjector {
    FieldId field_id;
    ValueType type;
};

struct ExprProjector {
    FieldId alias_field_id;
    Box<Expr> expr;
};

struct Limit {
    int limit;
};

struct Where {
    Box<Expr> condition;
};

struct OrderBy {
    std::vector<Expr> order_list;
    bool desc;
};

struct GroupBy {
    std::vector<Projector> group_list;
};

struct AdhocRelation {
    std::vector<Value> values;
    FieldId output_field_id;
};

struct SelectRelation {
    std::vector<Projector> projectors;

    Box<Relation> source;
    std::optional<Limit> limit;
    std::optional<Where> where;
    std::optional<OrderBy> order_by;
    std::optional<GroupBy> group_by;
    bool aggregate;
};

struct UnionAllRelation {
    Box<Relation> left;
    Box<Relation> right;
};

struct UnionAllSortedByRelation {
    Box<Relation> left;
    Box<Relation> right;
    OrderBy order_by;
};

struct FileRelation {
    std::string path;
};

struct FileIntervalRelation {
    std::string path;
    timestamp_t ts_from;
    timestamp_t ts_to;
};

struct NamedRelationReferenceRelation {
    std::string name;
};

struct MaterializeRelation {
    Box<Relation> relation;
};

struct Relation {
    RelationNode node;
    FieldSetNodePtr fields_out;
};

}  // namespace lsql::iface::sql::bind
