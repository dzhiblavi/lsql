#pragma once

#include <cstddef>
#include <iterator>

namespace lsql {

template <typename Tag = void>
struct IntrusiveListNode {
    IntrusiveListNode<Tag>* next_ = nullptr;
    IntrusiveListNode<Tag>* prev_ = nullptr;

    void unlink() {
        prev_->next_ = next_;
        next_->prev_ = prev_;
    }
};

template <typename T, typename Tag = void>
class IntrusiveList;

template <typename T, typename Tag>
struct IntrusiveListIterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;

    IntrusiveListIterator() = default;
    IntrusiveListIterator(IntrusiveListNode<Tag>* ptr) : node_(ptr) {}

    reference operator*() const { return *static_cast<T*>(node_); }

    pointer operator->() { return static_cast<T*>(node_); }

    IntrusiveListIterator& operator++() {
        node_ = node_->next_;
        return *this;
    }

    IntrusiveListIterator& operator--() {
        node_ = node_->prev_;
        return *this;
    }

    IntrusiveListIterator operator++(int) {
        auto tmp = *this;
        return ++tmp;
    }

    IntrusiveListIterator operator--(int) {
        auto tmp = *this;
        return --tmp;
    }

    bool operator==(const IntrusiveListIterator& a) const = default;

 private:
    IntrusiveListNode<Tag>* node_ = nullptr;

    friend class IntrusiveList<T, Tag>;
};

static_assert(std::bidirectional_iterator<IntrusiveListIterator<int, int>>);

template <typename T, typename Tag>
class IntrusiveList {
    using Node = IntrusiveListNode<Tag>;

 public:
    using iterator_type = IntrusiveListIterator<T, Tag>;
    using const_iterator_type = IntrusiveListIterator<const T, Tag>;

    IntrusiveList() = default;

    void push_back(T& item) {  // NOLINT
        insert(end(), item);
    }

    void push_front(T& item) {  // NOLINT
        insert(begin(), item);
    }

    void insert(iterator_type before, T& item) {
        Node* node = before.node_;
        Node* prev = node->prev_;

        link(prev, &item);
        link(&item, node);
    }

    void pop_back() {  // NOLINT
        erase(std::prev(end()));
    }

    void pop_front() {  // NOLINT
        erase(begin());
    }

    void erase(iterator_type iter) {
        Node* node = iter.node_;
        Node* prev = node->prev_;
        Node* next = node->next_;

        link(prev, next);
        node->next_ = node->prev_ = nullptr;
    }

    bool empty() const { return sentinel_.next_ == &sentinel_; }

    iterator_type begin() { return {sentinel_.next_}; }
    iterator_type end() { return {&sentinel_}; }

    const_iterator_type begin() const { return {sentinel_.next_}; }
    const_iterator_type end() const { return {&sentinel_}; }

 private:
    static void link(Node* a, Node* b) {
        a->next_ = b;
        b->prev_ = a;
    }

    Node sentinel_{&sentinel_, &sentinel_};
};

}  // namespace lsql
