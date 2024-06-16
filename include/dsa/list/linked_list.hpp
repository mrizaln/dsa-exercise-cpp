#pragma once

// NOTE: LinkedList implementation based on reference 1

#include <concepts>
#include <memory>

namespace dsa
{
    template <typename T>
    concept LinkedListElement = std::default_initializable<T> and (std::movable<T> or std::copyable<T>);

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

        LinkedList(const LinkedList& other);
        LinkedList& operator=(const LinkedList& other);

        void swap(LinkedList& other) noexcept;
        void clear() noexcept;

        T& insert(std::size_t pos, T&& element);
        T  remove(std::size_t pos);

        // snake-case to be able to use std functions like std::back_inserter
        T& push_front(T&& element);
        T& push_back(T&& element);
        T  pop_front();

        // NOTE: pop_back is a linear operation unlike the other operations, use with caution
        T pop_back();

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

    private:
        std::unique_ptr<Node> m_head = nullptr;
        Node*                 m_tail = nullptr;
        std::size_t           m_size = 0;
    };
}

#include "linked_list.inl"
