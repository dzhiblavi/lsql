#pragma once

#include "core/schema/FieldSet.h"
#include "core/types.h"

#include <vector>

namespace lsql::front::common::bound {

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
        (verify(children != nullptr), ...);
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

}  // namespace lsql::front::common::bound
