#pragma once

// NOTE: DualArrayDeque implementation based on reference 2

#include "array_list.hpp"
#include "stack.hpp"

#include <concepts>

namespace dsa
{
    template <typename T>
    concept DequeElement = ArrayElement<T>;

    // TODO: add random_access_iterator support
    template <DequeElement T>
    class Deque
    {
    public:
        template <bool IsConst>
        class [[nodiscard]] Iterator;

        friend class Iterator<false>;
        friend class Iterator<true>;

        using Element   = T;
        using Container = ArrayList<T>;
        using Backend   = Stack<ArrayList, T>;

        using value_type = Element;    // STL compliance

        Deque()  = default;
        ~Deque() = default;

        template <typename... Args>
            requires std::constructible_from<Container, Args...>
        Deque(Args&&... args)
            : m_front(args...)
            , m_back(std::forward<Args>(args)...)
        {
        }

        void swap(Deque& other) noexcept
            requires std::swappable<Container>;

        void clear() noexcept;

        T& push_back(T&& value);
        T& push_front(T&& value);
        T  pop_back();
        T  pop_front();

        std::size_t size() const noexcept { return m_front.size() + m_back.size(); }
        bool        empty() const noexcept { return m_front.empty() and m_back.empty(); }

        auto&& back(this auto&& self) noexcept;
        auto&& front(this auto&& self) noexcept;

        auto&& at(this auto&& self, std::size_t pos) noexcept;

        std::pair<Backend&, Backend&>             underlying() noexcept { return { m_front, m_back }; }
        std::pair<const Backend&, const Backend&> underlying() const noexcept { return { m_front, m_back }; }

    private:
        Backend m_front;    // stores index 0            .. size / 2 - 1 (reversed)
        Backend m_back;     // stores index size / 2 - 1 .. size - 1

        bool shouldBalance() const noexcept;
        void balance();
    };
}

// -----------------------------------------------------------------------------
// implementation detail
// -----------------------------------------------------------------------------

namespace dsa
{
    template <DequeElement T>
    void Deque<T>::swap(Deque& other) noexcept
        requires std::swappable<Container>
    {
        std::swap(m_front, other.m_front);
        std::swap(m_back, other.m_back);
    }

    template <DequeElement T>
    void Deque<T>::clear() noexcept
    {
        m_front.clear();
        m_back.clear();
    }

    template <DequeElement T>
    T& Deque<T>::push_back(T&& value)
    {
        auto& element = m_back.push(std::move(value));
        balance();
        return element;
    }

    template <DequeElement T>
    T& Deque<T>::push_front(T&& value)
    {
        auto& element = m_front.push(std::move(value));
        balance();
        return element;
    }

    template <DequeElement T>
    T Deque<T>::pop_back()
    {
        auto element = [this] {
            if (m_back.empty()) {
                return m_front.pop();
            } else {
                return m_back.pop();
            }
        }();
        balance();
        return element;
    }

    template <DequeElement T>
    T Deque<T>::pop_front()
    {
        auto element = [this] {
            if (m_front.empty()) {
                return m_back.pop();
            } else {
                return m_front.pop();
            }
        }();
        balance();
        return element;
    }

    template <DequeElement T>
    auto&& Deque<T>::back(this auto&& self) noexcept
    {
        if (self.m_back.empty()) {
            return self.m_front.top();
        } else {
            return self.m_back.top();
        }
    }

    template <DequeElement T>
    auto&& Deque<T>::front(this auto&& self) noexcept
    {
        if (self.m_front.empty()) {
            return self.m_back.top();
        } else {
            return self.m_front.top();
        }
    }

    template <DequeElement T>
    auto&& Deque<T>::at(this auto&& self, std::size_t pos) noexcept
    {
        auto& front = self.m_front.underlying();
        auto& back  = self.m_back.underlying();

        if (pos < front.size()) {
            return front.at(front.size() - pos - 1);
        } else {
            return back.at(pos - front.size());
        }
    }

    // NOTE: unless n < 2, each front and back contain at least n/4 elements. if this is not the case,
    //       then it moves elements betweeen them so that front and back contain exactly floor(n/2) each
    template <DequeElement T>
    bool Deque<T>::shouldBalance() const noexcept
    {
        return (3 * m_front.size() < m_back.size() || 3 * m_back.size() < m_front.size())
            && (m_front.size() + m_back.size()) >= 2;
    }

    // ensure the two stack from being too big or too small
    template <DequeElement T>
    void Deque<T>::balance()
    {
        if (!shouldBalance()) {
            return;
        }

        auto& front = m_front.underlying();
        auto& back  = m_back.underlying();

        auto totalSize    = front.size() + back.size();
        auto newFrontSize = totalSize / 2;

        ArrayList<T> newFront{};
        newFront.reserve(std::max(2 * newFrontSize, 1uz));
        for (std::size_t i = newFrontSize; i > 0; --i) {
            newFront.push_back(std::move(at(i - 1)));    // reversed
        }

        auto newBackSize = totalSize - newFrontSize;

        ArrayList<T> newBack{};
        newBack.reserve(std::max(2 * newBackSize, 1uz));
        for (std::size_t i = 0; i < newBackSize; ++i) {
            newBack.push_back(std::move(at(i + newFrontSize)));
        }

        front.swap(newFront);
        back.swap(newBack);
    }
}
