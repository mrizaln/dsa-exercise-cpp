#pragma once

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
        : m_size{ other.m_size }
    {
        for (const auto& element : other) {
            push_back(element);
        }
    }

    template <LinkedListElement T>
    LinkedList<T>& LinkedList<T>::operator=(LinkedList other)
    {
        swap(other);
        return *this;
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
            // current.reset(current->m_next.release());    // alternative of below but explicit
            current = std::move(current->m_next);
        }
        m_head = nullptr;
        m_tail = nullptr;
        m_size = 0;
    }

    template <LinkedListElement T>
    T& LinkedList<T>::insert(std::size_t pos, T&& element)
    {
        if (pos > m_size) {
            throw std::out_of_range{ "Index out of bounds" };
        }

        auto node = std::make_unique<Node>(std::move(element));

        if (pos == 0) {
            node->m_next = std::move(m_head);
            m_head       = std::move(node);
            m_tail       = m_head.get();
        } else if (pos == m_size) {
            m_tail->m_next = std::move(node);
            m_tail         = m_tail->m_next.get();
        } else {
            auto* prev = m_head.get();
            for (std::size_t i = 0; i < pos - 1; ++i) {
                prev = prev->m_next.get();
            }

            node->m_next = std::move(prev->m_next);
            prev->m_next = std::move(node);
        }

        ++m_size;
        return node->m_element;
    }

    template <LinkedListElement T>
    T LinkedList<T>::remove(std::size_t pos)
    {
        if (pos >= m_size) {
            throw std::out_of_range{ "Index out of bounds" };
        }

        if (pos == 0) {
            auto element = std::move(m_head->m_element);
            m_head       = std::move(m_head->m_next);
            --m_size;
            return element;
        } else {
            auto* prev = m_head.get();
            for (std::size_t i = 0; i < pos - 1; ++i) {
                prev = prev->m_next.get();
            }

            auto element = std::move(prev->m_next->m_element);
            prev->m_next = std::move(prev->m_next->m_next);
            --m_size;
            return element;
        }
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

    // TODO: make pop_back a constant time instead of linear to size
    // NOTE: to achieve constant time, m_tail probably needs to be changed to point to 'before tail'.
    //       also change the name to reflect the behavior.
    template <LinkedListElement T>
    T LinkedList<T>::pop_back()
    {
        if (m_head == nullptr) {
            throw std::out_of_range{ "List is empty" };
        }

        // one element
        if (m_head.get() == m_tail) {
            auto value = std::move(m_head->m_element);
            m_head     = nullptr;
            m_tail     = nullptr;
            m_size     = 0;
            return value;
        }

        Node* prev = nullptr;
        for (auto current = m_head.get(); current != m_tail; current = current->m_next.get()) {
            prev = current;
        }

        auto node = std::exchange(prev->m_next, nullptr);
        m_tail    = prev;
        --m_size;

        return std::move(node->m_element);
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

        reference operator*() const { return m_current->m_element; }
        pointer   operator->() const { return &m_current->m_element; }

    private:
        Node* m_current = nullptr;
    };

    // to make sure that the iterator are STL compliant
    namespace detail
    {
        struct LinkedListElementPlaceholder
        {
        };

        static_assert(std::forward_iterator<LinkedList<LinkedListElementPlaceholder>::Iterator<false>>);
        static_assert(std::forward_iterator<LinkedList<LinkedListElementPlaceholder>::Iterator<true>>);
    }
}
