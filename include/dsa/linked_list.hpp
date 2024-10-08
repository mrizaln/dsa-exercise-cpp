#pragma once

// NOTE: LinkedList implementation based on reference 1

#include "dsa/common.hpp"

#include <concepts>
#include <memory>

namespace dsa
{
    template <typename T>
    concept LinkedListElement = std::movable<T> or std::copyable<T>;

    template <LinkedListElement T>
    struct ListLink
    {
        T                         m_element;
        std::unique_ptr<ListLink> m_next = nullptr;
    };

    template <LinkedListElement T>
    class LinkedList
    {
    public:
        template <bool isConst>
        class [[nodiscard]] Iterator;    // forward iterator

        friend class Iterator<false>;
        friend class Iterator<true>;

        using Element = T;
        using Node    = ListLink<T>;

        using value_type = Element;    // STL compliance

        LinkedList() = default;
        ~LinkedList() { clear(); }

        LinkedList(LinkedList&& other) noexcept;
        LinkedList& operator=(LinkedList&& other) noexcept;

        LinkedList(const LinkedList& other)
            requires std::copyable<T>;
        LinkedList& operator=(const LinkedList& other)
            requires std::copyable<T>;

        void swap(LinkedList& other) noexcept;
        void clear() noexcept;

        T& insert(std::size_t pos, T&& element);
        T  remove(std::size_t pos);

        // snake-case to be able to use std functions like std::back_inserter
        T& push_front(T&& element);
        T& push_back(T&& element);
        T  pop_front();

        auto&& at(this auto&& self, std::size_t pos);
        auto&& node(this auto&& self, std::size_t pos);
        auto&& front(this auto&& self);
        auto&& back(this auto&& self);

        auto begin(this auto&& self) noexcept { return makeIter<Iterator, decltype(self)>(self.m_head.get()); }
        auto end(this auto&& self) noexcept { return makeIter<Iterator, decltype(self)>(nullptr);}

        Iterator<true> cbegin() const noexcept { return begin(); }
        Iterator<true> cend() const noexcept { return begin(); }

        std::size_t size() const noexcept { return m_size; }

    private:
        std::unique_ptr<Node> m_head = nullptr;
        Node*                 m_tail = nullptr;
        std::size_t           m_size = 0;
    };
}

// -----------------------------------------------------------------------------
// implementation detail
// -----------------------------------------------------------------------------

#include <stdexcept>
#include <utility>

namespace dsa
{
    template <LinkedListElement T>
    LinkedList<T>::LinkedList(LinkedList&& other) noexcept
        : m_head{ std::exchange(other.m_head, nullptr) }
        , m_tail{ std::exchange(other.m_tail, nullptr) }
        , m_size{ std::exchange(other.m_size, 0) }
    {
    }

    template <LinkedListElement T>
    LinkedList<T>& LinkedList<T>::operator=(LinkedList&& other) noexcept
    {
        if (this != &other) {
            clear();
            m_head = std::exchange(other.m_head, nullptr);
            m_tail = std::exchange(other.m_tail, nullptr);
            m_size = std::exchange(other.m_size, 0);
        }
        return *this;
    }

    template <LinkedListElement T>
    LinkedList<T>::LinkedList(const LinkedList& other)
        requires std::copyable<T>
        : m_size{ 0 }
    {
        for (auto element : other) {
            push_back(std::move(element));
        }
    }

    template <LinkedListElement T>
    LinkedList<T>& LinkedList<T>::operator=(const LinkedList& other)
        requires std::copyable<T>
    {
        if (this == &other) {
            return *this;
        }

        clear();
        for (auto element : other) {
            push_back(std::move(element));
        }
    }

    template <LinkedListElement T>
    void LinkedList<T>::swap(LinkedList& other) noexcept
    {
        std::swap(m_head, other.m_head);
        std::swap(m_tail, other.m_tail);
        std::swap(m_size, other.m_size);
    }

    template <LinkedListElement T>
    void LinkedList<T>::clear() noexcept
    {
        // prevent stack overflow from recursive destruction of nodes
        for (auto current = std::move(m_head); current != nullptr;) {
            current.reset(current->m_next.release());
        }
        m_head = nullptr;
        m_tail = nullptr;
        m_size = 0;
    }

    template <LinkedListElement T>
    T& LinkedList<T>::insert(std::size_t pos, T&& element)
    {
        if (pos > m_size) {
            throw std::out_of_range{ "Position out of bounds" };
        }

        if (pos == 0) {
            return push_front(std::move(element));
        } else if (pos == m_size) {
            return push_back(std::move(element));
        }

        auto* prev = &node(pos - 1);
        auto  node = std::make_unique<Node>(std::move(element));

        node->m_next = std::move(prev->m_next);
        prev->m_next = std::move(node);

        ++m_size;
        return prev->m_next->m_element;
    }

    template <LinkedListElement T>
    T LinkedList<T>::remove(std::size_t pos)
    {
        if (pos >= m_size) {
            throw std::out_of_range{ "Index out of bounds" };
        }

        if (pos == 0) {
            return pop_front();
        }

        auto* prev = &node(pos - 1);

        auto element = std::move(prev->m_next->m_element);
        prev->m_next = std::move(prev->m_next->m_next);

        --m_size;
        return element;
    }

    template <LinkedListElement T>
    T& LinkedList<T>::push_front(T&& element)
    {
        auto node = std::make_unique<Node>(std::move(element));

        if (m_head == nullptr) {
            m_head = std::move(node);
            m_tail = m_head.get();
        } else {
            node->m_next = std::move(m_head);
            m_head       = std::move(node);
        }

        ++m_size;
        return m_head->m_element;
    }

    template <LinkedListElement T>
    T& LinkedList<T>::push_back(T&& element)
    {
        auto node = std::make_unique<Node>(std::move(element));

        if (m_head == nullptr) {
            m_head = std::move(node);
            m_tail = m_head.get();
        } else {
            m_tail->m_next = std::move(node);
            m_tail         = m_tail->m_next.get();
        }

        ++m_size;
        return m_tail->m_element;
    }

    template <LinkedListElement T>
    T LinkedList<T>::pop_front()
    {
        if (m_head == nullptr) {
            throw std::out_of_range{ "List is empty" };
        }

        auto element = std::move(m_head->m_element);
        m_head       = std::move(m_head->m_next);
        --m_size;
        return element;
    }

    template <LinkedListElement T>
    auto&& LinkedList<T>::at(this auto&& self, std::size_t pos)
    {
        return self.node(pos).m_element;
    }

    template <LinkedListElement T>
    auto&& LinkedList<T>::node(this auto&& self, std::size_t pos)
    {
        if (pos >= self.m_size) {
            throw std::out_of_range{
                std::format("Position is out of range: pos {} on size {}", pos, self.m_size)
            };
        }

        auto* current = self.m_head.get();
        for (auto i = 0uz; i < pos; ++i) {
            current = current->m_next.get();
        }

        return *current;
    }

    template <LinkedListElement T>
    auto&& LinkedList<T>::front(this auto&& self)
    {
        if (self.m_size == 0) {
            throw std::out_of_range{ "LinkedList is empty" };
        }
        return deref<Node>(self.m_head).m_element;
    }

    template <LinkedListElement T>
    auto&& LinkedList<T>::back(this auto&& self)
    {
        if (self.m_size == 0) {
            throw std::out_of_range{ "LinkedList is empty" };
        }
        return deref<Node>(self.m_tail).m_element;
    }

    template <LinkedListElement T>
    template <bool IsConst>
    class LinkedList<T>::Iterator
    {
    public:
        // STL compatibility
        using iterator_category = std::forward_iterator_tag;
        using value_type        = typename LinkedList::Element;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<IsConst, const value_type*, value_type*>;
        using reference         = std::conditional_t<IsConst, const value_type&, value_type&>;

        // internal use
        using Node    = typename LinkedList::Node;
        using Element = value_type;

        Iterator() noexcept                      = default;
        Iterator(const Iterator&)                = default;
        Iterator& operator=(const Iterator&)     = default;
        Iterator(Iterator&&) noexcept            = default;
        Iterator& operator=(Iterator&&) noexcept = default;

        Iterator(Node* current)
            : m_current{ current }
        {
        }

        // for const iterator construction from iterator
        Iterator(Iterator<false>& other)
            : m_current{ other.m_current }
        {
        }

        // just a pointer comparison
        auto operator<=>(const Iterator&) const = default;

        Iterator& operator++()
        {
            m_current = m_current->m_next.get();
            return *this;
        }

        Iterator operator++(int)
        {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        Iterator& operator+=(difference_type n)
        {
            while (n-- > 0 && (m_current = m_current->m_next.get())) { }
        }

        reference operator*() const
        {
            if (m_current == nullptr) {
                throw std::out_of_range{ "Iterator is out of range" };
            }
            return m_current->m_element;
        }

        pointer operator->() const
        {
            if (m_current == nullptr) {
                throw std::out_of_range{ "Iterator is out of range" };
            }
            return &m_current->m_element;
        }

    private:
        Node* m_current = nullptr;
    };
}
