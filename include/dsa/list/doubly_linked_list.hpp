#pragma once

#include <concepts>
#include <memory>

namespace dsa
{
    template <typename T>
    concept DoublyLinkedListElement = std::default_initializable<T> and (std::movable<T> or std::copyable<T>);

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

        template <bool IsConst>
        using ReverseIterator = std::reverse_iterator<Iterator<IsConst>>;

        using Element = T;
        using Node    = DoublyListLink<T>;

        using value_type = Element;    // STL compliance

        DoublyLinkedList() = default;
        ~DoublyLinkedList() { clear(); }

        DoublyLinkedList(DoublyLinkedList&& other) noexcept;
        DoublyLinkedList& operator=(DoublyLinkedList&& other) noexcept;

        DoublyLinkedList(const DoublyLinkedList& other);
        DoublyLinkedList& operator=(const DoublyLinkedList& other);

        void swap(DoublyLinkedList& other) noexcept;
        void clear() noexcept;
        T&   insert(std::size_t pos, T&& element);
        T    remove(std::size_t pos);

        // snake-case to be able to use std functions like std::back_inserter
        T& push_front(T&& element);
        T& push_back(T&& element);
        T  pop_front();
        T  pop_back();

        // UB when m_head or m_tail nullptr
        T&       front() noexcept { return m_head->m_element; }
        T&       back() noexcept { return m_tail->m_element; }
        const T& front() const noexcept { return m_head->m_element; }
        const T& back() const noexcept { return m_tail->m_element; }

        std::size_t size() const noexcept { return m_size; }

        Iterator<false> begin() { return { m_head.get() }; }
        Iterator<false> end() { return { nullptr }; }
        Iterator<true>  begin() const { return { m_head.get() }; }
        Iterator<true>  end() const { return { nullptr }; }
        Iterator<true>  cbegin() const { return { m_head.get() }; }
        Iterator<true>  cend() const { return { nullptr }; }

        // reverse iterator
        ReverseIterator<false> rbegin() { return { m_head.get() }; }
        ReverseIterator<false> rend() { return { nullptr }; }
        ReverseIterator<true>  rbegin() const { return { m_head.get() }; }
        ReverseIterator<true>  rend() const { return { nullptr }; }
        ReverseIterator<true>  rcbegin() const { return { m_head.get() }; }
        ReverseIterator<true>  rcend() const { return { nullptr }; }

    private:
        std::unique_ptr<Node> m_head = nullptr;
        Node*                 m_tail = nullptr;
        std::size_t           m_size = 0;
    };
}

#include "doubly_linked_list.inl"
