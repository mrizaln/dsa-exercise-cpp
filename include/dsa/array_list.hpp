#pragma once

// NOTE: ArrayList implementation based on reference 1

#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>

namespace dsa
{
    template <typename T>
    concept ArrayElement = std::default_initializable<T> and (std::movable<T> or std::copyable<T>);

    template <ArrayElement T>
    class ArrayList
    {
    public:
        using Element  = T;
        using Iterator = T*;

        using value_type = Element;    // STL compliance

        ArrayList()  = default;
        ~ArrayList() = default;

        ArrayList(std::size_t capacity);

        ArrayList(const ArrayList& other);
        ArrayList& operator=(const ArrayList& other);

        ArrayList(ArrayList&& other) noexcept;
        ArrayList& operator=(ArrayList&& other) noexcept;

        void swap(ArrayList& other) noexcept;
        void clear() noexcept;

        T& insert(std::size_t pos, T&& element);
        T  remove(std::size_t pos);

        // snake-case to be able to use std functions like std::back_inserter
        T& push_back(T&& element);
        T  pop_back();

        T&       at(std::size_t pos);
        const T& at(std::size_t pos) const;

        std::size_t size() const noexcept { return m_size; }
        std::size_t capacity() const noexcept { return m_capacity; }

        T&       operator[](std::size_t pos) noexcept { return m_data[pos]; }
        const T& operator[](std::size_t pos) const noexcept { return m_data[pos]; }

        // UB when m_data is nullptr or m_size is 0
        T&       front() noexcept { return m_data[0]; }
        T&       back() noexcept { return m_data[m_size - 1]; }
        const T& front() const noexcept { return m_data[0]; }
        const T& back() const noexcept { return m_data[m_size - 1]; }

        T*             data() noexcept { return m_data.get(); }
        Iterator       begin() noexcept { return m_data.get(); }
        Iterator       end() noexcept { return m_data.get() + m_size; }
        const Iterator begin() const noexcept { return m_data.get(); }
        const Iterator end() const noexcept { return m_data.get() + m_size; }
        const Iterator cbegin() const noexcept { return m_data.get(); }
        const Iterator cend() const noexcept { return m_data.get() + m_size; }

    private:
        std::size_t m_capacity = 0;
        std::size_t m_size     = 0;

        std::unique_ptr<T[]> m_data = nullptr;

        void moveElement(std::size_t from, std::size_t to)
        {
            if constexpr (std::movable<T>) {
                m_data[to] = std::move(m_data[from]);
            } else {
                m_data[to] = m_data[from];
            }
        }
    };
}

// -----------------------------------------------------------------------------
// implementation detail
// -----------------------------------------------------------------------------

namespace dsa
{
    template <ArrayElement T>
    ArrayList<T>::ArrayList(std::size_t capacity)
        : m_capacity{ capacity }
        , m_data{ std::make_unique<T[]>(capacity) }
    {
    }

    template <ArrayElement T>
    ArrayList<T>::ArrayList(const ArrayList& other)
        : ArrayList{ other.m_capacity }
    {
        m_size = other.m_size;
        std::copy(other.m_data.get(), other.m_data.get() + m_size, m_data.get());
    }

    template <ArrayElement T>
    ArrayList<T>& ArrayList<T>::operator=(const ArrayList& other)
    {
        if (this == &other) {
            return *this;
        }

        clear();
        m_capacity = other.m_capacity;
        m_size     = other.m_size;
        std::copy(other.m_data.get(), other.m_data.get() + m_size, m_data.get());

        return *this;
    }

    template <ArrayElement T>
    ArrayList<T>::ArrayList(ArrayList&& other) noexcept
        : m_capacity{ std::exchange(other.m_capacity, 0) }
        , m_size{ std::exchange(other.m_size, 0) }
        , m_data{ std::exchange(other.m_data, nullptr) }
    {
    }

    template <ArrayElement T>
    ArrayList<T>& ArrayList<T>::operator=(ArrayList&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        clear();
        m_capacity = std::exchange(other.m_capacity, 0);
        m_size     = std::exchange(other.m_size, 0);
        m_data     = std::exchange(other.m_data, nullptr);

        return *this;
    }

    template <ArrayElement T>
    void ArrayList<T>::swap(ArrayList& other) noexcept
    {
        std::swap(m_capacity, other.m_capacity);
        std::swap(m_size, other.m_size);
        std::swap(m_data, other.m_data);
    }

    template <ArrayElement T>
    void ArrayList<T>::clear() noexcept
    {
        m_data = std::make_unique<T[]>(m_capacity);
        m_size = 0;
    };

    template <ArrayElement T>
    T& ArrayList<T>::insert(std::size_t pos, T&& element)
    {
        if (m_size == m_capacity) {
            throw std::out_of_range{ "List is full" };
        }

        if (pos > m_size) {
            throw std::out_of_range{ "Position is out of bounds" };
        }

        if (pos == m_size) {
            return push_back(std::move(element));
        }

        for (std::size_t i = m_size; i > pos; --i) {
            moveElement(i - 1, i);
        }

        ++m_size;
        return m_data[pos] = std::move(element);
    }

    template <ArrayElement T>
    T& ArrayList<T>::push_back(T&& element)
    {
        if (m_size == m_capacity) {
            throw std::out_of_range{ "List is full" };
        }

        return m_data[m_size++] = std::move(element);
    }

    template <ArrayElement T>
    T ArrayList<T>::remove(std::size_t pos)
    {
        if (m_size == 0) {
            throw std::out_of_range{ "List is empty" };
        }

        if (pos >= m_size) {
            throw std::out_of_range{ "Position is out of bounds" };
        }

        T removed = [&] {
            if constexpr (std::movable<T>) {
                return std::move(m_data[pos]);
            } else {
                return m_data[pos];
            }
        }();

        for (std::size_t i = pos; i < m_size - 1; ++i) {
            moveElement(i + 1, i);
        }

        m_size--;
        return removed;
    }

    template <ArrayElement T>
    T ArrayList<T>::pop_back()
    {
        if (m_size == 0) {
            throw std::out_of_range{ "List is empty" };
        }

        return m_data[--m_size];
    }

    template <ArrayElement T>
    T& ArrayList<T>::at(std::size_t pos)
    {
        if (pos >= m_size) {
            throw std::out_of_range{ "Position is out of bounds" };
        }

        return m_data[pos];
    }

    template <ArrayElement T>
    const T& ArrayList<T>::at(std::size_t pos) const
    {
        if (pos >= m_size) {
            throw std::out_of_range{ "Position is out of bounds" };
        }

        return m_data[pos];
    }
}
