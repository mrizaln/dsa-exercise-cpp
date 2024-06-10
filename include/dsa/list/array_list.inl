#pragma once

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
    ArrayList<T>& ArrayList<T>::operator=(const ArrayList other)
    {
        swap(other);
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
