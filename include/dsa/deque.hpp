#pragma once

// NOTE: DualArrayDeque implementation based on reference 2

#include "array_list.hpp"
#include "stack.hpp"

#include <concepts>

namespace dsa
{
    template <typename T>
    concept DequeElement = std::default_initializable<T> and (std::movable<T> or std::copyable<T>);

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

        // TODO: add random access support (operator[] and random_access_iterator)

        void swap(Deque& other) noexcept
            requires requires(Container c) { c.swap(c); } or std::swappable<Container>
        {
            if constexpr (requires(Container c) { c.swap(c); }) {
                m_front.swap(other.m_front);
                m_back.swap(other.m_back);
            } else {
                std::swap(m_front, other.m_front);
                std::swap(m_back, other.m_back);
            }
        }

        void clear() noexcept
        {
            m_front.clear();
            m_back.clear();
        }

        T& push_back(T&& value)
        {
            // auto& element = m_back.push(std::move(value));
            auto& element = [this, &value]() -> decltype(auto) {
                if (m_back.underlying().capacity() == m_back.size()) {
                    return m_front.push(std::move(value));
                } else {
                    return m_back.push(std::move(value));
                }
            }();
            balance();
            return element;
        }

        T& push_front(T&& value)
        {
            // auto& element = m_front.push(std::move(value));
            auto& element = [this, &value]() -> decltype(auto) {
                if (m_front.underlying().capacity() == m_front.size()) {
                    return m_back.push(std::move(value));
                } else {
                    return m_front.push(std::move(value));
                }
            }();
            balance();
            return element;
        }

        T pop_back()
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

        T pop_front()
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

        std::size_t size() const noexcept { return m_front.size() + m_back.size(); }
        bool        empty() const noexcept { return m_front.empty() and m_back.empty(); }

        T& back() noexcept
        {
            if (m_back.empty()) {
                return m_front.top();
            } else {
                return m_back.top();
            }
        }

        const T& back() const noexcept
        {
            if (m_back.empty()) {
                return m_front.top();
            } else {
                return m_back.top();
            }
        }

        T& front() noexcept
        {
            if (m_front.empty()) {
                return m_back.top();
            } else {
                return m_front.top();
            }
        }

        const T& front() const noexcept
        {
            if (m_front.empty()) {
                return m_back.top();
            } else {
                return m_front.top();
            }
        }

        std::pair<Backend&, Backend&>             underlying() noexcept { return { m_front, m_back }; }
        std::pair<const Backend&, const Backend&> underlying() const noexcept { return { m_front, m_back }; }

    private:
        Backend m_front;    // stores index 0            .. size / 2 - 1 (reversed)
        Backend m_back;     // stores index size / 2 - 1 .. size - 1

        // NOTE: unless n < 2, each front and back contain at least n/4 elements. if this is not the case,
        //       then it moves elements betweeen them so that front and back contain exactly floor(n/2) each
        bool shouldBalance() const noexcept
        {
            return (3 * m_front.size() < m_back.size() || 3 * m_back.size() < m_front.size())
                && (m_front.size() + m_back.size()) >= 2;
        }

        T&& get(std::size_t pos) noexcept
        {
            auto& front = m_front.underlying();
            auto& back  = m_back.underlying();

            if (pos < front.size()) {
                return std::move(front[front.size() - pos - 1]);
            } else {
                return std::move(back[pos - front.size()]);
            }
        }

        // ensure the two stack from being too big or too small
        void balance()
        {
            if (!shouldBalance()) {
                return;
            }

            auto& front = m_front.underlying();
            auto& back  = m_back.underlying();

            auto totalSize    = front.size() + back.size();
            auto newFrontSize = totalSize / 2;

            ArrayList<T> newFront{ front.capacity() };
            for (std::size_t i = newFrontSize; i > 0; --i) {
                newFront.push_back(get(i - 1));    // reversed
            }

            auto newBackSize = totalSize - newFrontSize;

            ArrayList<T> newBack{ back.capacity() };
            for (std::size_t i = 0; i < newBackSize; ++i) {
                newBack.push_back(get(i + newFrontSize));
            }

            front.swap(newFront);
            back.swap(newBack);
        }
    };
}
