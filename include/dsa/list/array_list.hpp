#pragma once

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
        T& front() noexcept { return m_data[0]; }
        T& back() noexcept { return m_data[m_size - 1]; }
        T& front() const noexcept { return m_data[0]; }
        T& back() const noexcept { return m_data[m_size - 1]; }

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

#include "array_list.inl"
