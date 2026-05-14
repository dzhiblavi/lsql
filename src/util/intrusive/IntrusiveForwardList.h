#pragma once

#include <cassert>
#include <cstddef>
#include <iterator>

namespace lsql::util {

template <typename Tag = void>
struct IntrusiveForwardListNode {
    IntrusiveForwardListNode<Tag>* next_ = nullptr;
};

template <typename T, typename Tag = void>
class IntrusiveForwardList;

template <typename T, typename Tag>
struct IntrusiveForwardListIterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;

    IntrusiveForwardListIterator() = default;
    IntrusiveForwardListIterator(IntrusiveForwardListNode<Tag>* ptr) : node_(ptr) {}

    reference operator*() const { return *static_cast<T*>(node_); }

    pointer operator->() { return static_cast<T*>(node_); }

    IntrusiveForwardListIterator& operator++() {
        node_ = node_->next_;
        return *this;
    }

    IntrusiveForwardListIterator operator++(int) {
        auto tmp = *this;
        return ++tmp;
    }

    bool operator==(const IntrusiveForwardListIterator& a) const = default;

 private:
    IntrusiveForwardListNode<Tag>* node_ = nullptr;

    friend class IntrusiveForwardList<T, Tag>;
};

static_assert(std::forward_iterator<IntrusiveForwardListIterator<int, int>>);

template <typename T, typename Tag>
class IntrusiveForwardList {
    using Node = IntrusiveForwardListNode<Tag>;

 public:
    using iterator_type = IntrusiveForwardListIterator<T, Tag>;

    IntrusiveForwardList() = default;

    void push_front(T& item) {
        item.next_ = head_;
        head_ = &item;
    }

    void pop_front() {
        assert(!empty());
        assert(head_ != nullptr);
        head_ = head_->next_;
    }

    bool empty() const { return head_ == nullptr; }

    T& front() {
        assert(head_ != nullptr);
        return *static_cast<T*>(head_);
    }

    const T& front() const {
        assert(head_ != nullptr);
        return *static_cast<const T*>(head_);
    }

 private:
    Node* head_ = nullptr;
};

}  // namespace lsql::util
