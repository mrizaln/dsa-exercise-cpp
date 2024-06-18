#pragma once

// NOTE: CircularArray implementation based on reference 1 and reference 2 (ArrayQueue)

#include "dsa/raw_buffer.hpp"

#include <algorithm>
#include <concepts>
#include <limits>
#include <optional>
#include <utility>

namespace dsa
{
    template <typename T>
    concept CircularBufferElement = std::movable<T> or std::copyable<T>;

    enum class BufferCapacityPolicy
    {
        FixedCapacity,
        DynamicCapacity,    // will double the capacity when full and halve when less than 1/4 full
    };

    enum class BufferStorePolicy
    {
        ReplaceOldest,
        ThrowOnFull,
    };

    enum class BufferResizePolicy
    {
        DiscardOld,
        DiscardNew,
    };

    struct BufferPolicy
    {
        BufferCapacityPolicy m_capacity = BufferCapacityPolicy::FixedCapacity;
        BufferStorePolicy    m_store    = BufferStorePolicy::ReplaceOldest;
    };

    // TODO: add the ability to push to/pop from front and add bidirectional_iterator support (it would be
    //       better if it would also support random_access_iterator)
    template <CircularBufferElement T>
    class CircularBuffer
    {
    public:
        template <bool IsConst>
        class [[nodiscard]] Iterator;    // forward iterator

        friend class Iterator<false>;
        friend class Iterator<true>;

        using Element = T;

        using value_type = Element;    // STL compliance

        CircularBuffer() = default;
        ~CircularBuffer() { clear(); };

        CircularBuffer(std::size_t capacity, BufferPolicy policy = {});

        CircularBuffer(CircularBuffer&& other) noexcept;
        CircularBuffer& operator=(CircularBuffer&& other) noexcept;

        CircularBuffer(const CircularBuffer& other)
            requires std::copyable<T>;
        CircularBuffer& operator=(const CircularBuffer& other)
            requires std::copyable<T>;

        void swap(CircularBuffer& other) noexcept;
        void reset() noexcept;
        void clear() noexcept;

        void resize(std::size_t newCapacity, BufferResizePolicy policy = BufferResizePolicy::DiscardOld);

        BufferPolicy getPolicy() const noexcept { return m_policy; };

        void setPolicy(
            std::optional<BufferCapacityPolicy> storagePolicy,
            std::optional<BufferStorePolicy>    storePolicy
        ) noexcept;

        // snake-case to be able to use std functions like std::back_inserter
        T& push_back(T&& value);
        T  pop_front();

        CircularBuffer& linearize() noexcept;

        // copied buffer will have the policy set using the parameter if it is not std::nullopt else it will
        // have the same policy as the original buffer
        [[nodiscard]] CircularBuffer linearizeCopy(std::optional<BufferPolicy> policy) const noexcept
            requires std::copyable<T>;

        std::size_t size() const noexcept;
        std::size_t capacity() const noexcept { return m_buffer.size(); }

        auto*  data(this auto&& self) noexcept { return self.m_buffer.data(); }
        auto&& front(this auto&& self) noexcept { return self.m_buffer.at(self.m_head); }
        auto&& back(this auto&& self) noexcept;

        Iterator<false> begin() noexcept { return { this, m_head }; }
        Iterator<false> end() noexcept { return { this, m_tail }; }
        Iterator<true>  begin() const noexcept { return { this, m_head }; }
        Iterator<true>  end() const noexcept { return { this, m_tail }; }
        Iterator<true>  cbegin() const noexcept { return { this, m_head }; }
        Iterator<true>  cend() const noexcept { return { this, m_tail }; }

    private:
        static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

        RawBuffer<T> m_buffer = {};
        std::size_t  m_head   = 0;
        std::size_t  m_tail   = npos;
        BufferPolicy m_policy = {};
    };
}

// -----------------------------------------------------------------------------
// implementation detail
// -----------------------------------------------------------------------------

namespace dsa
{
    template <CircularBufferElement T>
    CircularBuffer<T>::CircularBuffer(std::size_t capacity, BufferPolicy policy)
        : m_buffer{ capacity }
        , m_head{ 0 }
        , m_tail{ capacity == 0 ? npos : 0 }
        , m_policy{ policy }
    {
    }

    template <CircularBufferElement T>
    CircularBuffer<T>::CircularBuffer(const CircularBuffer& other)
        requires std::copyable<T>
        : m_buffer{ other.m_buffer.size() }
        , m_head{ other.m_head }
        , m_tail{ other.m_tail }
        , m_policy{ other.m_policy }
    {
        for (auto i = 0uz; i < size(); ++i) {
            m_buffer.construct(i, auto{ other.m_buffer.at(i) });
        }
    }

    template <CircularBufferElement T>
    CircularBuffer<T>& CircularBuffer<T>::operator=(const CircularBuffer& other)
        requires std::copyable<T>
    {
        if (this == &other) {
            return *this;
        }

        clear();

        swap(CircularBuffer{ other });    // copy-and-swap idiom
        return *this;
    }

    template <CircularBufferElement T>
    CircularBuffer<T>::CircularBuffer(CircularBuffer&& other) noexcept
        : m_buffer{ std::exchange(other.m_buffer, {}) }
        , m_head{ std::exchange(other.m_head, 0) }
        , m_tail{ std::exchange(other.m_tail, npos) }
        , m_policy{ std::exchange(other.m_policy, {}) }
    {
    }

    template <CircularBufferElement T>
    CircularBuffer<T>& CircularBuffer<T>::operator=(CircularBuffer&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        clear();

        m_buffer = std::exchange(other.m_buffer, {});
        m_head   = std::exchange(other.m_head, 0);
        m_tail   = std::exchange(other.m_tail, 0);
        m_policy = std::exchange(other.m_policy, {});

        return *this;
    }

    template <CircularBufferElement T>
    void CircularBuffer<T>::swap(CircularBuffer& other) noexcept
    {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_head, other.m_head);
        std::swap(m_tail, other.m_tail);
        std::swap(m_policy, other.m_policy);
    }

    template <CircularBufferElement T>
    void CircularBuffer<T>::reset() noexcept
    {
        m_head = 0;
        m_tail = 0;
    }

    template <CircularBufferElement T>
    void CircularBuffer<T>::clear() noexcept
    {
        for (auto i = 0uz; i < size(); ++i) {
            m_buffer.destroy((m_head + i) % size());
        }

        m_head = 0;
        m_tail = 0;
    }

    // TODO: add condition when
    // - size < capacity && size < newCapacity
    // - size < capacity && size > newCpacity
    template <CircularBufferElement T>
    void CircularBuffer<T>::resize(std::size_t newCapacity, BufferResizePolicy policy)
    {
        if (newCapacity == 0) {
            clear();
            m_tail = npos;
            return;
        }

        if (newCapacity == capacity()) {
            return;
        }

        if (size() == 0) {
            m_buffer = RawBuffer<T>{ newCapacity };
            m_head   = 0;
            m_tail   = 0;

            return;
        }

        if (newCapacity > capacity()) {
            RawBuffer<T> buffer{ newCapacity };
            for (auto i = 0uz; i < size(); ++i) {
                auto idx = (m_head + i) % size();
                buffer.construct(i, std::move(m_buffer.at(idx)));
                m_buffer.destroy(i);
            }

            m_tail   = m_tail == npos ? capacity() : (m_tail + capacity() - m_head) % capacity();
            m_buffer = std::move(buffer);
            m_head   = 0;

            return;
        }

        RawBuffer<T> buffer{ newCapacity };
        auto         count  = size();
        auto         offset = count <= newCapacity ? 0ul : count - newCapacity;

        switch (policy) {
        case BufferResizePolicy::DiscardOld: {
            auto begin = (m_head + offset) % capacity();
            for (auto i = 0uz; i < std::min(newCapacity, count); ++i) {
                auto idx = (begin + i) % capacity();
                buffer.construct(i, std::move(m_buffer.at(idx)));
                m_buffer.destroy(idx);
            }
        } break;
        case BufferResizePolicy::DiscardNew: {
            auto end = m_tail == npos ? m_head : m_tail;
            end      = (end + capacity() - offset) % capacity();
            for (auto i = std::min(newCapacity, count); i-- > 0;) {
                end = (end + capacity() - 1) % capacity();
                buffer.construct(i, std::move(m_buffer.at(end)));
                m_buffer.destroy(end);
            }
        } break;
        }

        m_buffer = std::move(buffer);
        m_head   = 0;
        m_tail   = count <= newCapacity ? count : npos;
    }

    template <CircularBufferElement T>
    void CircularBuffer<T>::setPolicy(
        std::optional<BufferCapacityPolicy> storagePolicy,
        std::optional<BufferStorePolicy>    storePolicy
    ) noexcept
    {
        if (storagePolicy.has_value()) {
            m_policy.m_capacity = storagePolicy.value();
        }

        if (storePolicy.has_value()) {
            m_policy.m_store = storePolicy.value();
        }
    }

    // snake-case to be able to use std functions like std::back_inserter
    template <CircularBufferElement T>
    T& CircularBuffer<T>::push_back(T&& value)
    {
        if (capacity() == 0) {
            throw std::logic_error{ "Can't push to a buffer with zero capacity" };
        }

        if (m_tail == npos && m_policy.m_capacity == BufferCapacityPolicy::DynamicCapacity) {
            resize(capacity() * 2, BufferResizePolicy::DiscardOld);
        }

        // m_tail is checked again here since call to resize() above will modify it
        if (m_tail == npos && m_policy.m_store == BufferStorePolicy::ThrowOnFull) {
            throw std::out_of_range{ "Buffer is full" };
        }

        auto current = m_head;

        // this branch only taken when the buffer is not full
        if (m_tail != npos) {
            current = m_tail;
            m_buffer.construct(current, std::move(value));    // new entry -> construct
            if (++m_tail == capacity()) {
                m_tail = 0;
            }
            if (m_tail == m_head) {
                m_tail = npos;
            }
        } else {
            current              = m_head;
            m_buffer.at(current) = std::move(value);    // already existing entry -> assign
            if (++m_head == capacity()) {
                m_head = 0;
            }
        }

        return m_buffer.at(current);
    }

    template <CircularBufferElement T>
    T CircularBuffer<T>::pop_front()
    {
        if (size() == 0) {
            throw std::out_of_range{ "Buffer is empty" };
        }

        auto value = std::move(m_buffer.at(m_head));
        m_buffer.destroy(m_head);

        if (m_tail == npos) {
            m_tail = m_head;
        }
        if (++m_head == capacity()) {
            m_head = 0;
        }

        if (m_policy.m_capacity == BufferCapacityPolicy::DynamicCapacity and size() == capacity() / 4) {
            resize(capacity() / 2, BufferResizePolicy::DiscardOld);
        }

        return value;
    }

    template <CircularBufferElement T>
    CircularBuffer<T>& CircularBuffer<T>::linearize() noexcept
    {
        if (m_head == 0 && m_tail == npos) {
            return *this;
        }

        std::rotate(m_buffer.data(), m_buffer.data() + m_head, m_buffer.data() + capacity());
        m_tail = m_tail != npos ? (m_tail + capacity() - m_head) % capacity() : npos;

        m_head = 0;

        return *this;
    }

    template <CircularBufferElement T>
    CircularBuffer<T> CircularBuffer<T>::linearizeCopy(std::optional<BufferPolicy> policy) const noexcept
        requires std::copyable<T>
    {
        CircularBuffer result{ capacity(), policy.value_or(m_policy) };

        std::rotate_copy(
            m_buffer.data(), m_buffer.data() + m_head, m_buffer.data() + capacity(), result.m_buffer.data()
        );

        result.m_tail = m_tail != npos ? (m_tail + capacity() - m_head) % capacity() : npos;
        result.m_head = 0;

        return result;
    }

    template <CircularBufferElement T>
    std::size_t CircularBuffer<T>::size() const noexcept
    {
        return m_tail == npos ? capacity() : (m_tail + capacity() - m_head) % capacity();
    }

    template <CircularBufferElement T>
    auto&& CircularBuffer<T>::back(this auto&& self) noexcept
    {
        return self.m_buffer.at(
            self.m_tail == npos    //
                ? (self.m_head + self.capacity() - 1) % self.capacity()
                : self.m_tail - 1
        );
    }

    template <CircularBufferElement T>
    template <bool IsConst>
    class CircularBuffer<T>::Iterator
    {
    public:
        // STL compatibility
        using iterator_category = std::forward_iterator_tag;
        using value_type        = typename CircularBuffer::Element;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<IsConst, const value_type*, value_type*>;
        using reference         = std::conditional_t<IsConst, const value_type&, value_type&>;

        using BufferPtr = std::conditional_t<IsConst, const CircularBuffer*, CircularBuffer*>;

        Iterator() noexcept                      = default;
        Iterator(const Iterator&)                = default;
        Iterator& operator=(const Iterator&)     = default;
        Iterator(Iterator&&) noexcept            = default;
        Iterator& operator=(Iterator&&) noexcept = default;

        Iterator(BufferPtr buffer, std::size_t current)
            : m_buffer{ buffer }
            , m_index{ current }
            , m_lastIndex{ m_index }
        {
        }

        // for const iterator construction from iterator
        Iterator(Iterator<false>& other)
            : m_buffer{ other.m_buffer }
            , m_index{ other.m_index }
            , m_lastIndex{ other.m_lastIndex }
        {
        }

        template <bool IsConst2>
        bool operator==(const Iterator<IsConst2>& other) const
        {
            return m_buffer == other.m_buffer && m_index == other.m_index;
        }

        Iterator& operator++()
        {
            if (m_index == CircularBuffer::npos) {
                return *this;
            }

            if (++m_index == m_buffer->capacity()) {
                m_index = 0;
            }

            if (m_index == m_lastIndex) {
                m_index = CircularBuffer::npos;
            }

            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        Iterator& operator+=(difference_type n)
        {
            while (n-- > 0 && m_index != CircularBuffer::npos) {
                ++(*this);
            }

            return *this;
        }

        // TODO: add bounds checking
        reference operator*() const { return m_buffer->m_buffer.at(m_index); };
        pointer   operator->() const { return &m_buffer->m_buffer.at(m_index); };

    private:
        BufferPtr   m_buffer    = nullptr;
        std::size_t m_index     = 0;
        std::size_t m_lastIndex = m_index;
    };
}
