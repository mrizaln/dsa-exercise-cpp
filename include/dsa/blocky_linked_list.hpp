#pragma once

// NOTE: BlockyLinkedList implementation based on reference 2 (SEList)

#include "dsa/circular_buffer.hpp"
#include "dsa/common.hpp"

#include <concepts>
#include <csignal>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#include <fmt/core.h>

namespace dsa
{
    template <typename T>
    concept BlockyLinkedListElement = CircularBufferElement<T>;

    template <BlockyLinkedListElement T>
    struct BlockyLinkedListLink
    {
        using Block = CircularBuffer<T>;

        std::unique_ptr<BlockyLinkedListLink> m_next  = nullptr;
        BlockyLinkedListLink*                 m_prev  = nullptr;
        Block                                 m_block = {};

        BlockyLinkedListLink(std::size_t capacity)
            : m_block{
                capacity,
                BufferPolicy{
                    .m_capacity = BufferCapacityPolicy::FixedCapacity,
                    .m_store    = BufferStorePolicy::ThrowOnFull,
                },
            }
        {
        }

        BlockyLinkedListLink(const BlockyLinkedListLink&)            = delete;
        BlockyLinkedListLink& operator=(const BlockyLinkedListLink&) = delete;
        BlockyLinkedListLink(BlockyLinkedListLink&&)                 = delete;
        BlockyLinkedListLink& operator=(BlockyLinkedListLink&&)      = delete;
    };

    template <BlockyLinkedListElement T>
    class BlockyLinkedList
    {
    public:
        template <bool IsConst>
        class [[nodiscard]] Iterator;

        friend class Iterator<false>;
        friend class Iterator<true>;

        template <bool IsConst>
        class [[nodiscard]] BlockIterator;

        friend class BlockIterator<false>;
        friend class BlockIterator<true>;

        using Element = T;
        using Node    = BlockyLinkedListLink<T>;

        using value_type = Element;    // STL compliance

        static constexpr std::size_t s_minimumBlockSize = 3;

        BlockyLinkedList() = default;
        ~BlockyLinkedList() { clear(); }

        BlockyLinkedList(std::size_t blockSize);

        BlockyLinkedList(BlockyLinkedList&& other) noexcept;
        BlockyLinkedList& operator=(BlockyLinkedList&& other) noexcept;

        BlockyLinkedList(const BlockyLinkedList& other)
            requires std::copyable<T>;
        BlockyLinkedList& operator=(const BlockyLinkedList& other)
            requires std::copyable<T>;

        void swap(BlockyLinkedList& other) noexcept;
        void clear() noexcept;
        T&   insert(std::size_t pos, T&& element);
        T    remove(std::size_t pos);

        // snake-case to be able to use std functions like std::back_inserter
        T& push_front(T&& element);
        T& push_back(T&& element);
        T  pop_front();
        T  pop_back();

        auto&& at(this auto&& self, std::size_t pos);
        auto&& front(this auto&& self);
        auto&& back(this auto&& self);

        std::size_t size() const noexcept { return m_size; }
        std::size_t blockSize() const noexcept { return m_blockSize; }

        auto begin(this auto&& self) noexcept;
        auto end(this auto&& self) noexcept;

        Iterator<true> cbegin() const noexcept { return begin(); }
        Iterator<true> cend() const noexcept { return begin(); }

        BlockIterator<true> head() const { return { m_head.get() }; }
        BlockIterator<true> tail() const { return { m_tail }; }

        auto blocks() const { return std::ranges::subrange{ head(), tail()++ }; }

        std::size_t canReach(Node& left, Node& right) const
        {
            auto count = 0uz;
            for (auto iterleft = &left; iterleft != nullptr; iterleft = iterleft->m_next.get()) {
                ++count;
                if (iterleft == &right) {
                    return count - 1;
                }
            }

            // count = 0;
            // for (auto iterright = &right; iterright != nullptr; iterright = iterright->m_prev) {
            //     ++count;
            //     if (iterright == &left) {
            //         return count;
            //     }
            // }

            return static_cast<std::size_t>(-1);
        }
        std::size_t canReachRight(Node& left, Node& right) const
        {
            auto count = 0uz;
            for (auto iterright = &right; iterright != nullptr; iterright = iterright->m_prev) {
                ++count;
                if (iterright == &left) {
                    return count;
                }
            }

            return static_cast<std::size_t>(-1);
        }

        bool isInList(Node& node)
        {
            for (auto* current = m_head.get(); current != nullptr; current = current->m_next.get()) {
                if (current == &node) {
                    return true;
                }
            }
            return false;
        }

    private:
        struct ElementLocation
        {
            Node&       m_node;
            std::size_t m_offset;
        };

        enum class InsertScenario
        {
            Shift,        // within r + 1 <= b steps found node u_r whose block is not full
            EndOfList,    // within r + 1 <= b steps reached end of list
            Spread,       // after b steps not find any block that is not full (all blocks size == b + 1)
        };

        enum class RemoveScenario
        {
            Shift,        // within r + 1 <= b steps found node whose block contains more than b - 1 elements
            EndOfList,    // within r + 1 <= b steps reached end of list
            Gather,       // after b steps not found any block containing more than b - 1 elements
        };

        std::unique_ptr<Node> m_head      = nullptr;
        Node*                 m_tail      = nullptr;
        std::size_t           m_size      = 0;
        std::size_t           m_blockSize = s_minimumBlockSize;

        ElementLocation locate(std::size_t pos) const;
        Node&           insertNodeAfter(Node& node);
        Node&           insertNodeBefore(Node& node);
        void            removeNode(Node& node);
        Node&           initHead();
        void            spread(Node& from, Node& until);
        void            gather(Node& node);

        std::pair<Node&, InsertScenario> determineInsertScenario(Node& node);
        std::pair<Node&, RemoveScenario> determineRemoveScenario(Node& node);
    };
}

// -----------------------------------------------------------------------------
// implementation detail
// -----------------------------------------------------------------------------

#include <format>
#include <stdexcept>

namespace dsa
{

    template <BlockyLinkedListElement T>
    BlockyLinkedList<T>::BlockyLinkedList(std::size_t blockSize)
        : m_blockSize{ blockSize }
    {
        if (blockSize < s_minimumBlockSize) {
            throw std::invalid_argument{ std::format("Block size must be at least {}", s_minimumBlockSize) };
        }
    }

    template <BlockyLinkedListElement T>
    BlockyLinkedList<T>::BlockyLinkedList(BlockyLinkedList&& other) noexcept
        : m_head{ std::exchange(other.m_head, nullptr) }
        , m_tail{ std::exchange(other.m_tail, nullptr) }
        , m_size{ std::exchange(other.m_size, 0) }
        , m_blockSize{ other.m_blockSize }
    {
    }

    template <BlockyLinkedListElement T>
    BlockyLinkedList<T>& BlockyLinkedList<T>::operator=(BlockyLinkedList&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        clear();
        m_head      = std::exchange(other.m_head, nullptr);
        m_tail      = std::exchange(other.m_tail, nullptr);
        m_size      = std::exchange(other.m_size, 0);
        m_blockSize = other.m_blockSize;

        return *this;
    }

    template <BlockyLinkedListElement T>
    BlockyLinkedList<T>::BlockyLinkedList(const BlockyLinkedList& other)
        requires std::copyable<T>
        : m_blockSize{ other.m_blockSize }
    {
        // TODO: implement more efficient copy implementation: per block
        for (auto element : other) {
            push_back(std::move(element));
        }
    }

    template <BlockyLinkedListElement T>
    BlockyLinkedList<T>& BlockyLinkedList<T>::operator=(const BlockyLinkedList& other)
        requires std::copyable<T>
    {
        if (this == &other) {
            return *this;
        }

        clear();
        // TODO: implement more efficient copy implementation: per block
        for (auto element : other) {
            push_back(std::move(element));
        }
    }

    template <BlockyLinkedListElement T>
    void BlockyLinkedList<T>::swap(BlockyLinkedList& other) noexcept
    {
        std::swap(m_head, other.m_head);
        std::swap(m_tail, other.m_tail);
        std::swap(m_size, other.m_size);
        std::swap(m_blockSize, other.m_blockSize);
    }

    template <BlockyLinkedListElement T>
    void BlockyLinkedList<T>::clear() noexcept
    {
        for (auto current = std::move(m_head); current != nullptr;) {
            current = std::move(current->m_next);
        }
        m_head = nullptr;
        m_tail = nullptr;
        m_size = 0;
    }

    template <BlockyLinkedListElement T>
    T& BlockyLinkedList<T>::insert(std::size_t pos, T&& element)
    {
        if (pos == m_size || (m_size == 0 && pos == 0)) {
            return push_back(std::move(element));
        }

        auto [node, offset]    = locate(pos);
        auto [rnode, scenario] = determineInsertScenario(node);    // rnode: node after r steps

        assert(isInList(node) and "node is not in the list! how can this happen?");
        assert(isInList(rnode) and "node is not in the list! how can this happen?");

        auto distLeft  = canReach(node, rnode);
        auto distRight = canReachRight(node, rnode);
        assert(
            distLeft != static_cast<std::size_t>(-1) and distRight != static_cast<std::size_t>(-1)
            and "newNode should be reachable"
        );

        switch (scenario) {
        case InsertScenario::Spread: {
            spread(node, rnode);
            break;
        }
        case InsertScenario::EndOfList: [[fallthrough]];
        case InsertScenario::Shift: {
            auto count = 0uz;
            for (Node* current = &rnode; current != &node; current = current->m_prev) {
                assert(current != nullptr && "node should not be null");
                Node* prev = current->m_prev;
                assert(prev != nullptr && "node should not be null");
                current->m_block.push_front(std::move(prev->m_block.pop_back()));
                ++count;
            }
        } break;
        }

        ++m_size;
        return node.m_block.insert(offset, std::move(element));
    }

    template <BlockyLinkedListElement T>
    T BlockyLinkedList<T>::remove(std::size_t pos)
    {
        auto [node, offset]    = locate(pos);
        auto [rnode, scenario] = determineRemoveScenario(node);

        if (scenario == RemoveScenario::Gather) {
            gather(node);
        }

        T value = [&] {
            try {
                return node.m_block.remove(offset);
            } catch (std::exception& e) {
                fmt::println("exception: {}", e.what());
                std::raise(SIGTRAP);
                throw;
            }
        }();
        --m_size;

        Node* current = &node;

        while (current->m_block.size() < m_blockSize - 1 && current->m_next != nullptr) {
            Node* next = current->m_next.get();
            current->m_block.push_back(std::move(next->m_block.pop_front()));
            current = current->m_next.get();
        }

        if (current->m_block.size() == 0) {
            removeNode(*current);
        }

        return value;
    }

    template <BlockyLinkedListElement T>
    T& BlockyLinkedList<T>::push_front(T&& element)
    {
        return insert(0, std::move(element));
    }

    template <BlockyLinkedListElement T>
    T& BlockyLinkedList<T>::push_back(T&& element)
    {
        if (m_head == nullptr) {
            auto& head = initHead();
            ++m_size;
            return head.m_block.push_back(std::move(element));
        }

        if (m_tail->m_block.size() == m_blockSize + 1) {
            auto& newTail = insertNodeAfter(*m_tail);
            ++m_size;
            return newTail.m_block.push_back(std::move(element));
        } else {
            ++m_size;
            return m_tail->m_block.push_back(std::move(element));
        }
    }

    template <BlockyLinkedListElement T>
    T BlockyLinkedList<T>::pop_front()
    {
        return remove(0);
    }

    template <BlockyLinkedListElement T>
    T BlockyLinkedList<T>::pop_back()
    {
        return remove(m_size - 1);
    }

    template <BlockyLinkedListElement T>
    auto&& BlockyLinkedList<T>::at(this auto&& self, std::size_t pos)
    {
        auto [node, offset] = self.locate(pos);
        return node.m_block.at(offset);
    }

    template <BlockyLinkedListElement T>
    auto&& BlockyLinkedList<T>::front(this auto&& self)
    {
        return self.at(0);
    }

    template <BlockyLinkedListElement T>
    auto&& BlockyLinkedList<T>::back(this auto&& self)
    {
        return self.at(self.m_size - 1);
    }

    template <BlockyLinkedListElement T>
    auto BlockyLinkedList<T>::begin(this auto&& self) noexcept
    {
        return makeIter<Iterator, decltype(self)>(&self, 0uz);
    }

    template <BlockyLinkedListElement T>
    auto BlockyLinkedList<T>::end(this auto&& self) noexcept
    {
        return makeIter<Iterator, decltype(self)>(&self, self.size());
    }

    template <BlockyLinkedListElement T>
    BlockyLinkedList<T>::ElementLocation BlockyLinkedList<T>::locate(std::size_t pos) const
    {
        if (pos >= m_size) {
            throw std::out_of_range{
                std::format("Position is out of range: pos {} on size {}", pos, m_size)
            };
        }

        if (pos <= m_size / 2) {
            Node* current     = m_head.get();
            auto  currentSize = current->m_block.size();

            while (pos >= currentSize) {
                pos         -= currentSize;
                current      = current->m_next.get();
                currentSize  = current->m_block.size();
                assert(current != nullptr && "current node must not be null");
            }

            if (pos >= m_blockSize + 1) {
                std::raise(SIGTRAP);
            }

            return { *current, pos };
        } else {
            Node* current = m_tail;
            auto  idx     = m_size - current->m_block.size();

            while (pos < idx) {
                current  = current->m_prev;
                idx     -= current->m_block.size();
                assert(current != nullptr && "current node must not be null");
            }

            if ((pos - idx) >= m_blockSize + 1) {
                std::raise(SIGTRAP);
            }

            return { *current, pos - idx };
        }
    }

    template <BlockyLinkedListElement T>
    BlockyLinkedList<T>::Node& BlockyLinkedList<T>::insertNodeAfter(Node& node)
    {
        auto* prev = &node;
        auto  next = std::move(prev->m_next);

        auto newNode = std::make_unique<Node>(m_blockSize + 1);
        if (next != nullptr) {
            next->m_prev = newNode.get();
        }

        newNode->m_prev = prev;
        newNode->m_next = std::move(next);

        prev->m_next = std::move(newNode);

        if (&node == m_tail) {
            m_tail = node.m_next.get();
        }

        return *prev->m_next;
    }

    template <BlockyLinkedListElement T>
    BlockyLinkedList<T>::Node& BlockyLinkedList<T>::insertNodeBefore(Node& node)
    {
        Node* prev = node.m_prev;
        auto  next = std::move(&node == m_head.get() ? m_head : prev->m_next);

        auto newNode = std::make_unique<Node>(m_blockSize + 1);
        if (next != nullptr) {
            next->m_prev = newNode.get();
        }

        newNode->m_prev = prev;
        newNode->m_next = std::move(next);

        if (&node == m_head.get()) {
            m_head = std::move(newNode);
            return *m_head;
        } else {
            prev->m_next = std::move(newNode);
            return *prev->m_next;
        }
    }

    template <BlockyLinkedListElement T>
    void BlockyLinkedList<T>::removeNode(Node& node)
    {
        if (&node == m_head.get()) {
            m_head = std::move(node.m_next);
            if (m_head != nullptr) {
                m_head->m_prev = nullptr;
            }
        } else {
            if (&node == m_tail) {
                m_tail = node.m_prev;
            }

            auto next = std::move(node.m_next);
            if (next != nullptr) {
                next->m_prev = node.m_prev;
            }

            auto& prev  = *node.m_prev;
            prev.m_next = std::move(next);
        }
    }

    template <BlockyLinkedListElement T>
    BlockyLinkedList<T>::Node& BlockyLinkedList<T>::initHead()
    {
        m_head = std::make_unique<Node>(m_blockSize + 1);
        m_tail = m_head.get();
        return *m_head;
    }

    template <BlockyLinkedListElement T>
    void BlockyLinkedList<T>::spread(Node& from, Node& until)
    {
        Node* node = &from;
        for (auto i = 0uz; i < m_blockSize; ++i) {
            node = node->m_next.get();
        }
        assert(node == &until);
        auto& newNode = insertNodeBefore(until);

        auto distLeft  = canReach(from, newNode);
        auto distRight = canReachRight(from, newNode);

        assert(
            distLeft != static_cast<std::size_t>(-1) and distRight != static_cast<std::size_t>(-1)
            and "newNode should be reachable"
        );

        auto count = 0uz;
        for (Node* current = &newNode; current != &from; current = current->m_prev) {
            assert(current != nullptr && "node should not be null");
            auto innercount = 0uz;
            while (current->m_block.size() < m_blockSize) {
                Node* prev = current->m_prev;
                assert(prev != nullptr && "node should not be null");
                current->m_block.push_front(std::move(prev->m_block.pop_back()));
                ++innercount;
            }
            ++count;
        }
    }

    template <BlockyLinkedListElement T>
    void BlockyLinkedList<T>::gather(Node& node)
    {
        Node* current = &node;
        for (auto i = 0uz; i < m_blockSize - 1; ++i) {
            while (current->m_block.size() < m_blockSize) {
                Node* next = current->m_next.get();
                current->m_block.push_back(std::move(next->m_block.pop_front()));
            }
            current = current->m_next.get();
        }
        removeNode(*current);
    }

    template <BlockyLinkedListElement T>
    std::pair<typename BlockyLinkedList<T>::Node&, typename BlockyLinkedList<T>::InsertScenario>
    BlockyLinkedList<T>::determineInsertScenario(Node& node)
    {
        std::size_t steps   = 0;
        Node*       current = &node;

        while (steps < m_blockSize && current != nullptr && current->m_block.size() == m_blockSize + 1) {
            current = current->m_next.get();
            ++steps;
        }

        if (current == nullptr) {
            insertNodeAfter(*m_tail);
            return { *m_tail, InsertScenario::EndOfList };
        } else if (steps == m_blockSize) {
            assert(current != nullptr && "node should not be null");
            assert(canReachRight(node, *current));
            return { *current, InsertScenario::Spread };
        } else {
            assert(current != nullptr && "node should not be null");
            assert(canReachRight(node, *current));
            return { *current, InsertScenario::Shift };
        }
    }

    template <BlockyLinkedListElement T>
    std::pair<typename BlockyLinkedList<T>::Node&, typename BlockyLinkedList<T>::RemoveScenario>
    BlockyLinkedList<T>::determineRemoveScenario(Node& node)
    {
        std::size_t steps   = 0;
        Node*       current = &node;

        while (steps < m_blockSize && current != nullptr && current->m_block.size() == m_blockSize - 1) {
            current = current->m_next.get();
            ++steps;
        }

        if (current == nullptr) {
            return { *m_tail, RemoveScenario::EndOfList };
        } else if (steps == m_blockSize) {
            return { *current, RemoveScenario::Gather };
        } else {
            return { *current, RemoveScenario::Shift };
        }
    }

    template <BlockyLinkedListElement T>
    template <bool IsConst>
    class BlockyLinkedList<T>::Iterator
    {
    public:
        // STL compatibility
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = typename BlockyLinkedList::Element;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<IsConst, const value_type*, value_type*>;
        using reference         = std::conditional_t<IsConst, const value_type&, value_type&>;

        // internal use
        using Node    = typename BlockyLinkedList::Node;
        using ListPtr = std::conditional_t<IsConst, const BlockyLinkedList*, BlockyLinkedList*>;

        Iterator() noexcept                      = default;
        Iterator(const Iterator&)                = default;
        Iterator& operator=(const Iterator&)     = default;
        Iterator(Iterator&&) noexcept            = default;
        Iterator& operator=(Iterator&&) noexcept = default;

        Iterator(ListPtr list, std::size_t pos)
            : m_list{ list }
            , m_pos{ pos }
        {
        }

        Iterator(Iterator<false>& other)
            : m_list{ other.m_list }
            , m_pos{ other.m_pos }
        {
        }

        auto operator<=>(const Iterator&) const = default;

        Iterator& operator+=(difference_type n)
        {
            // casted n possibly become very large if it was negative, but when it was added to m_pos, m_pos
            // will wraparound anyway since it was unsigned
            m_pos += static_cast<std::size_t>(n);

            // detect wrap-around
            if (m_pos >= m_list->size()) {
                m_pos = m_list->size();
            }

            return *this;
        }

        Iterator& operator-=(difference_type n) { return (*this) += -n; }

        Iterator& operator++() { return (*this) += 1; }
        Iterator& operator--() { return (*this) -= 1; }

        Iterator operator++(int)
        {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        Iterator operator--(int)
        {
            auto copy = *this;
            --(*this);
            return copy;
        }

        reference operator*() const
        {
            if (m_list == nullptr || m_pos == m_list->size()) {
                throw std::out_of_range{ "Iterator is out of range" };
            }
            return m_list->at(m_pos);
        };

        pointer operator->() const
        {
            if (m_list == nullptr || m_pos == m_list->size()) {
                throw std::out_of_range{ "Iterator is out of range" };
            }

            return &m_list->at(m_pos);
        };

    private:
        ListPtr     m_list = nullptr;
        std::size_t m_pos  = 0;
    };

    template <BlockyLinkedListElement T>
    template <bool IsConst>
    class BlockyLinkedList<T>::BlockIterator
    {
    public:
        // STL compatibility
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = typename BlockyLinkedList::Node::Block;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<IsConst, const value_type*, value_type*>;
        using reference         = std::conditional_t<IsConst, const value_type&, value_type&>;

        // internal use
        using Node    = typename BlockyLinkedList::Node;
        using Element = value_type;

        BlockIterator() noexcept                           = default;
        BlockIterator(const BlockIterator&)                = default;
        BlockIterator& operator=(const BlockIterator&)     = default;
        BlockIterator(BlockIterator&&) noexcept            = default;
        BlockIterator& operator=(BlockIterator&&) noexcept = default;

        BlockIterator(Node* current)
            : m_current{ current }
        {
        }

        // for const iterator construction from iterator
        BlockIterator(BlockIterator<false>& other)
            : m_current{ other.m_current }
        {
        }

        // just a pointer comparison
        auto operator<=>(const BlockIterator&) const = default;

        BlockIterator& operator++()
        {
            m_current = m_current->m_next.get();
            return *this;
        }

        BlockIterator operator++(int)
        {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        BlockIterator& operator+=(difference_type n)
        {
            if (n < 0) {
                return (*this) -= -n;
            } else {
                auto i = static_cast<std::size_t>(n);
                while (i-- > 0 && (m_current = m_current->m_next.get())) { }
            }
        }

        BlockIterator& operator--()
        {
            m_current = m_current->m_prev;
            return *this;
        }

        BlockIterator operator--(int)
        {
            auto copy = *this;
            --(*this);
            return copy;
        }

        BlockIterator& operator-=(difference_type n)
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
                throw std::out_of_range{ "BlockIterator is out of range" };
            }

            return m_current->m_block;
        }

        pointer operator->() const
        {
            if (m_current == nullptr) {
                throw std::out_of_range{ "BlockIterator is out of range" };
            }

            return &m_current->m_block;
        }

    private:
        Node* m_current = nullptr;
    };
}
