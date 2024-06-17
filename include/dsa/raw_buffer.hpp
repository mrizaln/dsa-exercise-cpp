#pragma once

#include <cassert>
#include <cstddef>
#include <algorithm>
#include <memory>
#include <utility>

namespace dsa
{
    // an encapsulation of a raw buffer/memory that propagates the constness of the buffer to the elements
    template <typename T>
    class RawBuffer
    {
    public:
        RawBuffer() = default;

        explicit RawBuffer(std::size_t size)
            : m_data{ m_allocator.allocate(size) }
            , m_size{ size }
        {
#ifndef NDEBUG
            std::fill(m_constructed.get(), m_constructed.get() + size, false);
#endif
        }

        ~RawBuffer()
        {
            if (m_data) {
#ifndef NDEBUG
                assert(
                    std::all_of(
                        m_constructed.get(),
                        m_constructed.get() + m_size,
                        [](bool constructed) { return !constructed; }
                    )
                    && "Not all elements are destructed"
                );
#endif
                m_allocator.deallocate(m_data, m_size);
            }
        }

        RawBuffer(RawBuffer&& other) noexcept
            : m_data{ std::exchange(other.m_data, nullptr) }
            , m_size{ std::exchange(other.m_size, 0) }
#ifndef NDEBUG
            , m_constructed{ std::exchange(other.m_constructed, nullptr) }
#endif
        {
        }

        RawBuffer& operator=(RawBuffer&& other) noexcept
        {
            if (this == &other) {
                return *this;
            }

            if (m_data) {
                m_allocator.deallocate(m_data, m_size);
            }

            m_data = std::exchange(other.m_data, nullptr);
#ifndef NDEBUG
            m_constructed = std::exchange(other.m_constructed, nullptr);
#endif
            m_size = std::exchange(other.m_size, 0);

            return *this;
        }

        RawBuffer(const RawBuffer&)            = delete;
        RawBuffer& operator=(const RawBuffer&) = delete;

        template <typename... Ts>
        T& construct(std::size_t offset, Ts&&... args) noexcept(std::is_nothrow_constructible_v<T, Ts...>)
        {
#ifndef NDEBUG
            assert(!m_constructed[offset] && "Element not constructed");
            m_constructed[offset] = true;
#endif
            return *std::construct_at(m_data + offset, std::forward<Ts>(args)...);
        }

        T destroy(std::size_t offset) noexcept
        {
#ifndef NDEBUG
            assert(m_constructed[offset] && "Element not constructed");
            m_constructed[offset] = false;
#endif
            T element = std::move(m_data[offset]);
            std::move(m_data + offset + 1, m_data + m_size, m_data + offset);
            std::destroy_at(m_data + m_size - 1);
            return element;
        }

        T*       data() noexcept { return m_data; }
        const T* data() const noexcept { return m_data; }

        T&       at(std::size_t pos) noexcept { return m_data[pos]; }
        const T& at(std::size_t pos) const noexcept { return m_data[pos]; }

        std::size_t size() const noexcept { return m_size; }

    private:
        [[no_unique_address]] std::allocator<T> m_allocator = {};

        T*          m_data = nullptr;
        std::size_t m_size = 0;

#ifndef NDEBUG
        std::unique_ptr<bool[]> m_constructed = std::make_unique<bool[]>(m_size);
#endif
    };
}
