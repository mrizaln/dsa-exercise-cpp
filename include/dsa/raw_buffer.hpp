#pragma once

#include <cassert>
#include <cstddef>
#include <algorithm>
#include <memory>
#include <utility>

#ifndef DSA_RAW_BUFFER_DEBUG
#    ifdef NDEBUG
#        define DSA_RAW_BUFFER_DEBUG 0
#    else
#        define DSA_RAW_BUFFER_DEBUG 1
#    endif
#endif

namespace dsa
{
    // an encapsulation of a raw buffer/memory that propagates the constness of the buffer to the elements
    template <typename T>
    class RawBuffer
    {
    public:
        RawBuffer() = default;

        explicit RawBuffer(std::size_t size);
        ~RawBuffer();

        RawBuffer(RawBuffer&& other) noexcept;
        RawBuffer& operator=(RawBuffer&& other) noexcept;

        RawBuffer(const RawBuffer&)            = delete;
        RawBuffer& operator=(const RawBuffer&) = delete;

        template <typename... Ts>
        T& construct(std::size_t offset, Ts&&... args) noexcept(std::is_nothrow_constructible_v<T, Ts...>);

        void destroy(std::size_t offset) noexcept;

        T*       data() noexcept { return m_data; }
        const T* data() const noexcept { return m_data; }

        T&       at(std::size_t pos) & noexcept { return m_data[pos]; }
        T&&      at(std::size_t pos) && noexcept { return std::move(m_data[pos]); }
        const T& at(std::size_t pos) const noexcept { return m_data[pos]; }

        std::size_t size() const noexcept { return m_size; }

    private:
        [[no_unique_address]] std::allocator<T> m_allocator = {};

        T*          m_data = nullptr;
        std::size_t m_size = 0;

#if DSA_RAW_BUFFER_DEBUG
        std::unique_ptr<bool[]> m_constructed = std::make_unique<bool[]>(m_size);
#endif
    };
}

// -----------------------------------------------------------------------------
// implementation detail
// -----------------------------------------------------------------------------

namespace dsa
{
    template <typename T>
    RawBuffer<T>::RawBuffer(std::size_t size)
        : m_data{ m_allocator.allocate(size) }
        , m_size{ size }
    {
        if constexpr (DSA_RAW_BUFFER_DEBUG) {
            std::fill(m_constructed.get(), m_constructed.get() + size, false);
        }
    }

    template <typename T>
    RawBuffer<T>::~RawBuffer()
    {
        if (m_data == nullptr) {
            return;
        }

        if constexpr (DSA_RAW_BUFFER_DEBUG) {
            assert(
                std::all_of(
                    m_constructed.get(),
                    m_constructed.get() + m_size,
                    [](bool constructed) { return !constructed; }
                )
                && "Not all elements are destructed"
            );
        }
        m_allocator.deallocate(m_data, m_size);
        m_data = nullptr;
    }

    template <typename T>
    RawBuffer<T>::RawBuffer(RawBuffer&& other) noexcept
        : m_data{ std::exchange(other.m_data, nullptr) }
        , m_size{ std::exchange(other.m_size, 0) }
#if DSA_RAW_BUFFER_DEBUG
        , m_constructed{ std::exchange(other.m_constructed, nullptr) }
#endif
    {
    }

    template <typename T>
    RawBuffer<T>& RawBuffer<T>::operator=(RawBuffer&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        if (m_data) {
            m_allocator.deallocate(m_data, m_size);
        }

        m_data = std::exchange(other.m_data, nullptr);
        m_size = std::exchange(other.m_size, 0);

        if constexpr (DSA_RAW_BUFFER_DEBUG) {
            m_constructed = std::exchange(other.m_constructed, nullptr);
        }

        return *this;
    }

    template <typename T>
    template <typename... Ts>
    T& RawBuffer<T>::construct(
        std::size_t offset,
        Ts&&... args
    ) noexcept(std::is_nothrow_constructible_v<T, Ts...>)
    {
        if constexpr (DSA_RAW_BUFFER_DEBUG) {
            assert(!m_constructed[offset] && "Element not constructed");
            m_constructed[offset] = true;
        }
        return *std::construct_at(m_data + offset, std::forward<Ts>(args)...);
    }

    template <typename T>
    void RawBuffer<T>::destroy(std::size_t offset) noexcept
    {
        if constexpr (DSA_RAW_BUFFER_DEBUG) {
            assert(m_constructed[offset] && "Element not constructed");
            m_constructed[offset] = false;
        }
        std::destroy_at(m_data + offset);
    }
}
