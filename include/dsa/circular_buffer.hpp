#pragma once

// NOTE: CircularArray implementation based on reference 1 and reference 2 (ArrayQueue)

#include <algorithm>
#include <concepts>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace dsa
{
    template <typename T>
    concept CircularBufferElement = std::default_initializable<T> and (std::movable<T> or std::copyable<T>);

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
    // TODO: relaxed the std::default_initializable constraint: use RawBuffer
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

        CircularBuffer()  = default;
        ~CircularBuffer() = default;

        CircularBuffer(std::size_t capacity, BufferPolicy policy = {})
            : m_buffer{ std::make_unique<T[]>(capacity) }
            , m_capacity{ capacity }
            , m_head{ 0 }
            , m_tail{ capacity == 0 ? npos : 0 }
            , m_policy{ policy }
        {
        }

        CircularBuffer(const CircularBuffer& other)
            requires std::copyable<T>
            : m_buffer{ std::make_unique<T[]>(other.m_capacity) }
            , m_capacity{ other.m_capacity }
            , m_head{ other.m_head }
            , m_tail{ other.m_tail }
            , m_policy{ other.m_policy }
        {
            std::copy(other.m_buffer.get(), other.m_buffer.get() + other.m_capacity, m_buffer.get());
        }

        CircularBuffer& operator=(const CircularBuffer& other)
            requires std::copyable<T>
        {
            if (this == &other) {
                return *this;
            }

            swap(CircularBuffer{ other });    // copy-and-swap idiom
            return *this;
        }

        CircularBuffer(CircularBuffer&& other) noexcept
            : m_buffer{ std::exchange(other.m_buffer, nullptr) }
            , m_capacity{ std::exchange(other.m_capacity, 0) }
            , m_head{ std::exchange(other.m_head, 0) }
            , m_tail{ std::exchange(other.m_tail, npos) }
            , m_policy{ std::exchange(other.m_policy, {}) }
        {
        }

        CircularBuffer& operator=(CircularBuffer&& other) noexcept
        {
            if (this == &other) {
                return *this;
            }

            m_buffer   = std::exchange(other.m_buffer, nullptr);
            m_capacity = std::exchange(other.m_capacity, 0);
            m_head     = std::exchange(other.m_head, 0);
            m_tail     = std::exchange(other.m_tail, 0);
            m_policy   = std::exchange(other.m_policy, {});

            return *this;
        }

        void swap(CircularBuffer& other) noexcept
        {
            std::swap(m_buffer, other.m_buffer);
            std::swap(m_capacity, other.m_capacity);
            std::swap(m_head, other.m_head);
            std::swap(m_tail, other.m_tail);
            std::swap(m_policy, other.m_policy);
        }

        void reset() noexcept
        {
            m_head = 0;
            m_tail = 0;
        }

        void clear() noexcept
        {
            m_head = 0;
            m_tail = 0;

            if constexpr (std::is_trivially_default_constructible_v<T>) {
                std::fill(m_buffer.get(), m_buffer.get() + m_capacity, T{});
            } else {
                for (std::size_t i = 0; i < m_capacity; ++i) {
                    m_buffer[i] = T{};
                }
            }
        }

        // TODO: add condition when
        // - size < capacity && size < newCapacity
        // - size < capacity && size > newCpacity
        void resize(std::size_t newCapacity, BufferResizePolicy policy = BufferResizePolicy::DiscardOld)
        {
            if (newCapacity == 0) {
                clear();
                m_capacity = 0;
                m_tail     = npos;
                return;
            }

            if (newCapacity == capacity()) {
                return;
            }

            if (size() == 0) {
                m_buffer   = std::make_unique<T[]>(newCapacity);
                m_capacity = newCapacity;
                m_head     = 0;
                m_tail     = 0;

                return;
            }

            if (newCapacity > capacity()) {
                const auto b = [buf = m_buffer.get()](std::size_t offset) {
                    return std::move_iterator{ buf + offset };
                };

                auto buffer = std::make_unique<T[]>(newCapacity);
                std::rotate_copy(b(0), b(m_head), b(capacity()), buffer.get());
                m_buffer   = std::move(buffer);
                m_tail     = m_tail == npos ? capacity() : (m_tail + capacity() - m_head) % capacity();
                m_head     = 0;
                m_capacity = newCapacity;

                return;
            }

            auto buffer = std::make_unique<T[]>(newCapacity);
            auto count  = size();
            auto offset = count <= newCapacity ? 0ul : count - newCapacity;

            switch (policy) {
            case BufferResizePolicy::DiscardOld: {
                auto begin = (m_head + offset) % capacity();
                for (std::size_t i = 0; i < std::min(newCapacity, count); ++i) {
                    buffer[i] = std::move(m_buffer[(begin + i) % capacity()]);
                }
            } break;
            case BufferResizePolicy::DiscardNew: {
                auto end = m_tail == npos ? m_head : m_tail;
                end      = (end + capacity() - offset) % capacity();
                for (std::size_t i = std::min(newCapacity, count); i-- > 0;) {
                    end       = (end + capacity() - 1) % capacity();
                    buffer[i] = std::move(m_buffer[end]);
                }
            } break;
            }

            m_buffer   = std::move(buffer);
            m_capacity = newCapacity;
            m_head     = 0;
            m_tail     = count <= newCapacity ? count : npos;
        }

        BufferPolicy getPolicy() const noexcept { return m_policy; }

        void setPolicy(
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
        T& push_back(T&& value)
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
                current           = m_tail;
                m_buffer[current] = std::move(value);
                if (++m_tail == capacity()) {
                    m_tail = 0;
                }
                if (m_tail == m_head) {
                    m_tail = npos;
                }
            } else {
                current           = m_head;
                m_buffer[current] = std::move(value);
                if (++m_head == capacity()) {
                    m_head = 0;
                }
            }

            return m_buffer[current];
        }

        T pop_front()
        {
            if (size() == 0) {
                throw std::out_of_range{ "Buffer is empty" };
            }

            auto value = std::move(m_buffer[m_head]);
            if (m_tail == npos) {
                m_tail = m_head;
            }
            if (++m_head == capacity()) {
                m_head = 0;
            }

            if (size() == capacity() / 4) {
                resize(capacity() / 2, BufferResizePolicy::DiscardOld);
            }

            return value;
        }

        CircularBuffer& linearize() noexcept
        {
            if (m_head == 0 && m_tail == npos) {
                return *this;
            }

            std::rotate(m_buffer.get(), m_buffer.get() + m_head, m_buffer.get() + capacity());
            m_tail = m_tail != npos ? (m_tail + capacity() - m_head) % capacity() : npos;

            m_head = 0;

            return *this;
        }

        [[nodiscard]] CircularBuffer linearizeCopy() const noexcept
            requires std::copyable<T>
        {
            CircularBuffer result{ m_capacity };
            std::rotate_copy(
                m_buffer.get(), m_buffer.get() + m_head, m_buffer.get() + capacity(), result.m_buffer.get()
            );

            result.m_tail = m_tail != npos ? (m_tail + m_capacity - m_head) % m_capacity : npos;
            result.m_head = 0;

            return result;
        }

        std::size_t size() const noexcept
        {
            return m_tail == npos ? capacity() : (m_tail + capacity() - m_head) % capacity();
        }

        std::size_t capacity() const noexcept { return m_capacity; }

        T&       front() noexcept { return m_buffer[m_head]; }
        const T& front() const noexcept { return m_buffer[m_head]; }

        T& back() noexcept
        {
            return m_buffer[m_tail == npos ? (m_head + capacity() - 1) % capacity() : m_tail - 1];
        }

        const T& back() const noexcept
        {
            return m_buffer[m_tail == npos ? (m_head + capacity() - 1) % capacity() : m_tail - 1];
        }

        T*       data() noexcept { return m_buffer.get(); }
        const T* data() const noexcept { return m_buffer.get(); }

        Iterator<false> begin() noexcept { return { this, m_head }; }
        Iterator<false> end() noexcept { return { this, m_tail }; }
        Iterator<true>  begin() const noexcept { return { this, m_head }; }
        Iterator<true>  end() const noexcept { return { this, m_tail }; }
        Iterator<true>  cbegin() const noexcept { return { this, m_head }; }
        Iterator<true>  cend() const noexcept { return { this, m_tail }; }

    private:
        static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

        std::unique_ptr<T[]> m_buffer   = nullptr;
        std::size_t          m_capacity = 0;
        std::size_t          m_head     = 0;
        std::size_t          m_tail     = npos;
        BufferPolicy         m_policy   = {};
    };

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
        reference operator*() const { return m_buffer->m_buffer[m_index]; };
        pointer   operator->() const { return &m_buffer->m_buffer[m_index]; };

    private:
        BufferPtr   m_buffer    = nullptr;
        std::size_t m_index     = 0;
        std::size_t m_lastIndex = m_index;
    };
};
