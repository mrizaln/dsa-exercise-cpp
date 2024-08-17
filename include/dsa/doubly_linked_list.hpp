#pragma once

// NOTE: DoublyLinkedList implementation based on reference 1

#include "dsa/common.hpp"

#include <concepts>
#include <memory>

namespace dsa
{
    template <typename T>
    concept DoublyLinkedListElement = std::movable<T> or std::copyable<T>;

    template <DoublyLinkedListElement T>
    struct DoublyListLink
    {
        T                               m_element;
        std::unique_ptr<DoublyListLink> m_next = nullptr;
        DoublyListLink*                 m_prev = nullptr;
    };

    template <DoublyLinkedListElement T>
    class DoublyLinkedList
    {
    public:
        template <bool IsConst>
        class [[nodiscard]] Iterator;    // bidirectional iterator

        friend class Iterator<false>;
        friend class Iterator<true>;

        using Element = T;
        using Node    = DoublyListLink<T>;

        using value_type = Element;    // STL compliance

        DoublyLinkedList() = default;
        ~DoublyLinkedList() { clear(); }

        DoublyLinkedList(DoublyLinkedList&& other) noexcept;
        DoublyLinkedList& operator=(DoublyLinkedList&& other) noexcept;

        DoublyLinkedList(const DoublyLinkedList& other)
            requires std::copyable<T>;
        DoublyLinkedList& operator=(const DoublyLinkedList& other)
            requires std::copyable<T>;

        void swap(DoublyLinkedList& other) noexcept;
        void clear() noexcept;
        T&   insert(std::size_t pos, T&& element);
        T    remove(std::size_t pos);

        // snake-case to be able to use std functions like std::back_inserter
        T& push_front(T&& element);
        T& push_back(T&& element);
        T  pop_front();
        T  pop_back();

        auto&& at(this auto&& self, std::size_t pos);
        auto&& node(this auto&& self, std::size_t pos);
        auto&& front(this auto&& self);
        auto&& back(this auto&& self);

        std::size_t size() const noexcept { return m_size; }

        auto begin(this auto&& self) noexcept
        {
            return makeIter<Iterator, decltype(self)>(self.m_head.get());
        }
        auto end(this auto&& self) noexcept { return makeIter<Iterator, decltype(self)>(nullptr); }

        Iterator<true> cbegin() const noexcept { return begin(); }
        Iterator<true> cend() const noexcept { return begin(); }

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
    template <DoublyLinkedListElement T>
    DoublyLinkedList<T>::DoublyLinkedList(DoublyLinkedList&& other) noexcept
        : m_head{ std::exchange(other.m_head, nullptr) }
        , m_tail{ std::exchange(other.m_tail, nullptr) }
        , m_size{ std::exchange(other.m_size, 0) }
    {
    }

    template <DoublyLinkedListElement T>
    DoublyLinkedList<T>& DoublyLinkedList<T>::operator=(DoublyLinkedList&& other) noexcept
    {
        if (this != &other) {
            clear();
            m_head = std::exchange(other.m_head, nullptr);
            m_tail = std::exchange(other.m_tail, nullptr);
            m_size = std::exchange(other.m_size, 0);
        }
        return *this;
    }

    template <DoublyLinkedListElement T>
    DoublyLinkedList<T>::DoublyLinkedList(const DoublyLinkedList& other)
        requires std::copyable<T>
    {
        for (auto element : other) {
            push_back(std::move(element));
        }
    }

    template <DoublyLinkedListElement T>
    DoublyLinkedList<T>& DoublyLinkedList<T>::operator=(const DoublyLinkedList& other)
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

    template <DoublyLinkedListElement T>
    void DoublyLinkedList<T>::swap(DoublyLinkedList& other) noexcept
    {
        std::swap(m_head, other.m_head);
        std::swap(m_tail, other.m_tail);
        std::swap(m_size, other.m_size);
    }

    template <DoublyLinkedListElement T>
    void DoublyLinkedList<T>::clear() noexcept
    {
        // prevent stack overflow from recursive destruction of nodes
        for (auto current = std::move(m_head); current != nullptr;) {
            current.reset(current->m_next.release());
        }
        m_head = nullptr;
        m_tail = nullptr;
        m_size = 0;
    }

    template <DoublyLinkedListElement T>
    T& DoublyLinkedList<T>::insert(std::size_t pos, T&& element)
    {
        if (pos > m_size) {
            throw std::out_of_range{ "Position out of range" };
        }

        if (pos == 0) {
            return push_front(std::move(element));
        } else if (pos == m_size) {
            return push_back(std::move(element));
        }

        auto* prev = &node(pos - 1);
        auto  next = std::move(prev->m_next);

        auto node = std::make_unique<Node>(std::move(element));

        node->m_prev = prev;
        node->m_next = std::move(next);

        prev->m_next = std::move(node);

        ++m_size;
        return prev->m_next->m_element;
    }

    template <DoublyLinkedListElement T>
    T DoublyLinkedList<T>::remove(std::size_t pos)
    {
        if (pos >= m_size) {
            throw std::out_of_range{ "Index out of bounds" };
        }

        if (pos == 0) {
            return pop_front();
        } else if (pos == m_size - 1) {
            return pop_back();
        }

        auto* prev = &node(pos - 1);

        auto [element, next, _] = std::move(*prev->m_next);
        next->m_prev            = prev;
        prev->m_next            = std::move(next);

        --m_size;
        return std::move(element);    // NRVO not working?
    }

    template <DoublyLinkedListElement T>
    T& DoublyLinkedList<T>::push_front(T&& element)
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

    template <DoublyLinkedListElement T>
    T& DoublyLinkedList<T>::push_back(T&& element)
    {
        auto node = std::make_unique<Node>(std::move(element));

        if (m_head == nullptr) {
            m_head = std::move(node);
            m_tail = m_head.get();
        } else {
            node->m_prev   = m_tail;
            m_tail->m_next = std::move(node);
            m_tail         = m_tail->m_next.get();
        }

        ++m_size;
        return m_tail->m_element;
    }

    template <DoublyLinkedListElement T>
    T DoublyLinkedList<T>::pop_front()
    {
        if (m_head == nullptr) {
            throw std::out_of_range{ "List is empty" };
        }

        auto [element, next, _] = std::move(*m_head);
        m_head                  = std::move(next);

        --m_size;
        return std::move(element);    // NRVO not working?
    }

    template <DoublyLinkedListElement T>
    T DoublyLinkedList<T>::pop_back()
    {
        if (m_head == nullptr) {
            throw std::out_of_range{ "List is empty" };
        } else if (m_size == 1) {
            return pop_front();
        }

        auto* prev            = m_tail->m_prev;
        auto [element, _, __] = std::move(*prev->m_next);
        prev->m_next          = nullptr;
        m_tail                = prev;

        --m_size;
        return std::move(element);    // NRVO not working?
    }

    template <DoublyLinkedListElement T>
    auto&& DoublyLinkedList<T>::at(this auto&& self, std::size_t pos)
    {
        return self.node(pos).m_element;
    }

    template <DoublyLinkedListElement T>
    auto&& DoublyLinkedList<T>::node(this auto&& self, std::size_t pos)
    {
        if (pos >= self.m_size) {
            throw std::out_of_range{
                std::format("Position is out of range: pos {} on size {}", pos, self.m_size)
            };
        }

        if (pos <= self.m_size / 2) {
            auto* current = self.m_head.get();
            for (auto i = 0uz; i < pos; ++i) {
                current = current->m_next.get();
            }
            return *current;
        } else {
            auto* current = self.m_tail;
            for (auto i = self.m_size - 1; i > pos; --i) {
                current = current->m_prev;
            }
            return *current;
        }
    }

    template <DoublyLinkedListElement T>
    auto&& DoublyLinkedList<T>::front(this auto&& self)
    {
        if (self.m_size == 0) {
            throw std::out_of_range{ "LinkedList is empty" };
        }
        return derefConst<Node>(self.m_head).m_element;
    }

    template <DoublyLinkedListElement T>
    auto&& DoublyLinkedList<T>::back(this auto&& self)
    {
        if (self.m_size == 0) {
            throw std::out_of_range{ "LinkedList is empty" };
        }
        return derefConst<Node>(self.m_tail).m_element;
    }

    template <DoublyLinkedListElement T>
    template <bool IsConst>
    class DoublyLinkedList<T>::Iterator
    {
    public:
        // STL compatibility
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = typename DoublyLinkedList::Element;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<IsConst, const value_type*, value_type*>;
        using reference         = std::conditional_t<IsConst, const value_type&, value_type&>;

        // internal use
        using Node    = typename DoublyLinkedList::Node;
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
            auto copy = *this;
            ++(*this);
            return copy;
        }

        Iterator& operator+=(difference_type n)
        {
            if (n < 0) {
                return (*this) -= -n;
            } else {
                auto i = static_cast<std::size_t>(n);
                while (i-- > 0 && (m_current = m_current->m_next.get())) { }
            }
        }

        Iterator& operator--()
        {
            m_current = m_current->m_prev;
            return *this;
        }

        Iterator operator--(int)
        {
            auto copy = *this;
            --(*this);
            return copy;
        }

        Iterator& operator-=(difference_type n)
        {
            if (n < 0) {
                return (*this) += -n;
            } else {
                auto i = static_cast<std::size_t>(n);
                while (i-- > 0 && (m_current = m_current->m_prev)) { }
            }
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
