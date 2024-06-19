#pragma once

// NOTE: ArrayList implementation based on reference 1

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <utility>

#include "raw_buffer.hpp"

namespace dsa
{
    template <typename T>
    concept ArrayElement = std::movable<T> or std::copyable<T>;

    template <ArrayElement T>
    class ArrayList
    {
    public:
        using Element  = T;
        using Iterator = T*;

        using value_type = Element;    // STL compliance

        ArrayList() = default;
        ~ArrayList() { clear(); }

        explicit ArrayList(std::size_t count)
            requires std::default_initializable<T>;

        ArrayList(ArrayList&& other) noexcept;
        ArrayList& operator=(ArrayList&& other) noexcept;

        ArrayList(const ArrayList& other)
            requires std::copyable<T>;
        ArrayList& operator=(const ArrayList& other)
            requires std::copyable<T>;

        void swap(ArrayList& other) noexcept;
        void clear() noexcept;

        T& insert(std::size_t pos, T&& element);
        T  remove(std::size_t pos);

        T& push_back(T&& value) { return insert(m_size, std::move(value)); }
        T  pop_back() { return remove(m_size - 1); }

        // reallocation will happen in order to fit
        void fit();

        // resize the capacity:
        // if count > capacity() -> reallocate the buffer with the new capacity.
        // else                  -> do nothing.
        void reserve(std::size_t count);

        auto&& at(this auto&& self, std::size_t pos)
        {
            if (pos >= self.m_size) {
                throw std::out_of_range{ "Cannot access element at position greater than or equal to size" };
            }
            return self.m_buffer.at(pos);
        }

        auto&& front(this auto&& self) { return self.at(0); }
        auto&& back(this auto&& self) { return self.at(self.m_size - 1); }

        auto* data(this auto&& self) { return self.m_buffer.data(); }

        auto* begin(this auto&& self) { return self.m_buffer.data(); }
        auto* end(this auto&& self) { return self.m_buffer.data() + self.m_size; }

        std::size_t size() const noexcept { return m_size; }
        std::size_t capacity() const noexcept { return m_buffer.size(); }

    private:
        struct ShiftResult
        {
            std::size_t begin;
            std::size_t count;
        };

        RawBuffer<T> m_buffer = {};
        std::size_t  m_size   = 0;

        ShiftResult shiftRight(std::size_t begin, std::size_t count);
        void        destroyAndShiftLeft(std::size_t begin, std::size_t end);
        void        grow();
    };
}

// -----------------------------------------------------------------------------
// implementation detail
// -----------------------------------------------------------------------------

namespace dsa
{
    template <ArrayElement T>
    ArrayList<T>::ArrayList(std::size_t count)
        requires std::default_initializable<T>
        : m_buffer{ count }
        , m_size{ count }
    {
        for (auto i = 0uz; i < count; ++i) {
            m_buffer.construct(i);
        }
    }

    template <ArrayElement T>
    ArrayList<T>::ArrayList(ArrayList&& other) noexcept
        : m_buffer{ std::exchange(other.m_buffer, {}) }
        , m_size{ std::exchange(other.m_size, 0) }
    {
    }

    template <ArrayElement T>
    ArrayList<T>& ArrayList<T>::operator=(ArrayList&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        clear();

        m_buffer = std::exchange(other.m_buffer, {});
        m_size   = std::exchange(other.m_size, 0);

        return *this;
    }

    template <ArrayElement T>
    ArrayList<T>::ArrayList(const ArrayList& other)
        requires std::copyable<T>
        : m_buffer{ other.m_buffer.size() }
        , m_size{ other.m_size }
    {
        for (std::size_t i = 0; i < m_size; ++i) {
            m_buffer.construct(i, auto{ other.m_buffer.at(i) });
        }
    }

    template <ArrayElement T>
    ArrayList<T>& ArrayList<T>::operator=(const ArrayList& other)
        requires std::copyable<T>
    {
        if (this == &other) {
            return *this;
        }

        clear();

        m_buffer = RawBuffer<T>{ other.m_buffer.size() };    // destroy the old buffer and create a new one
        m_size   = other.m_size;

        for (std::size_t i = 0; i < m_size; ++i) {
            m_buffer.construct(i, other.m_buffer.at(i));
        }

        return *this;
    }

    template <ArrayElement T>
    void ArrayList<T>::swap(ArrayList& other) noexcept
    {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_size, other.m_size);
    }

    template <ArrayElement T>
    void ArrayList<T>::clear() noexcept
    {
        for (std::size_t i = 0; i < m_size; ++i) {
            m_buffer.destroy(i);
        }
        m_size = 0;
    }

    template <ArrayElement T>
    T& ArrayList<T>::insert(std::size_t pos, T&& element)
    {
        if (pos > m_size) {
            throw std::out_of_range{
                std::format("Cannot insert at position greater than size ({} > {})", pos, m_size)
            };
        }

        if (m_size == capacity()) {
            grow();
        }

        if (pos < m_size) {
            auto [begin, _] = shiftRight(pos, 1);
            ++m_size;
            return m_buffer.at(begin) = std::move(element);
        } else {
            ++m_size;
            return m_buffer.construct(pos, std::move(element));
        }
    }

    template <ArrayElement T>
    T ArrayList<T>::remove(std::size_t pos)
    {
        if (pos >= m_size) {
            throw std::out_of_range{ "Cannot remove at position greater than or equal to size" };
        }

        auto value = std::move(m_buffer.at(pos));
        destroyAndShiftLeft(pos, pos + 1);
        --m_size;
        return value;
    }

    template <ArrayElement T>
    void ArrayList<T>::fit()
    {
        RawBuffer<T> newBuffer{ m_size };
        for (auto i = 0uz; i < m_size; ++i) {
            newBuffer.construct(i, std::move(m_buffer.at(i)));
            m_buffer.destroy(i);
        }
        m_buffer = std::move(newBuffer);
    }

    template <ArrayElement T>
    void ArrayList<T>::reserve(std::size_t count)
    {
        if (count > capacity()) {
            RawBuffer<T> newBuffer{ count };
            for (auto i = 0uz; i < m_size; ++i) {
                newBuffer.construct(i, std::move(m_buffer.at(i)));
                m_buffer.destroy(i);
            }
            m_buffer = std::move(newBuffer);
        }
    }

    // the elements between [begin, begin + count) will be shifted to the right by count positions.
    // the gap then returned and in a moved-from state ready to be moved-to (moved assignment).
    template <ArrayElement T>
    ArrayList<T>::ShiftResult ArrayList<T>::shiftRight(std::size_t begin, std::size_t count)
    {
        assert(count > 0 && "Cannot shift right by 0");

        if (begin > m_size) {
            throw std::out_of_range{ "Cannot shift right at position greater than size" };
        }

        if (m_size + count > capacity()) {
            throw std::out_of_range{ "Cannot shift right, not enough capacity" };
        }

        auto placementEnd = m_size + count;

        // placement new-ed elements
        for (auto offset = placementEnd - 1; offset >= placementEnd - count; --offset) {
            m_buffer.construct(offset, std::move(m_buffer.at(offset - count)));
        }

        // the rest of the elements
        auto movedCount = (m_size - begin) - count;
        std::move_backward(
            m_buffer.data() + begin,
            m_buffer.data() + begin + movedCount,
            m_buffer.data() + placementEnd - count
        );

        return { begin, count };
    }

    // the elements between [begin, end) will be destroyed.
    // the rest of the elements then will be shifted to the left filling the gap.
    template <ArrayElement T>
    void ArrayList<T>::destroyAndShiftLeft(std::size_t begin, std::size_t end)
    {
        assert(begin < end && "Begin pointer must be less than end pointer");

        if (end > m_size) {
            throw std::out_of_range{ "Cannot destroy and shift left at position greater than size" };
        }

        // instead of directly destroying the [begin, end) elements, we will reassign the elements from
        // [end, m_size) to [begin, m_size - (end - begin)) and then destroy the rest of the elements.

        // move the elements to the left
        auto movedBegin = end;
        auto movedEnd   = m_size;

        std::move(m_buffer.data() + movedBegin, m_buffer.data() + movedEnd, m_buffer.data() + begin);

        // remove the rest of the elements
        for (auto offset = m_size - (end - begin); offset < m_size; ++offset) {
            m_buffer.destroy(offset);
        }
    };

    template <ArrayElement T>
    void ArrayList<T>::grow()
    {
        // 2 * growth factor
        auto newSize = capacity() == 0 ? 1 : 2 * capacity();
        reserve(newSize);
    }
}
