#pragma once

#include <concepts>
#include <utility>

namespace dsa
{
    template <template <typename> typename C, typename T>
    concept QueueCompatible = requires(C<T> c, const C<T> cc, T&& t) {
        { cc.back() } -> std::same_as<const T&>;
        { cc.front() } -> std::same_as<const T&>;
        { cc.size() } -> std::same_as<std::size_t>;
        { c.back() } -> std::same_as<T&>;
        { c.front() } -> std::same_as<T&>;
        { c.push_back(std::move(t)) } -> std::same_as<T&>;
        { c.pop_front() } -> std::same_as<T>;
    };

    template <template <typename> typename C, typename T>
        requires QueueCompatible<C, T>
    class Queue
    {
    public:
        using Container = C<T>;
        using Element   = T;

        template <typename... Args>
            requires std::constructible_from<Container, Args...>
        Queue(Args&&... args)
            : m_container(std::forward<Args>(args)...)
        {
        }

        bool        empty() const { return m_container.size() == 0; }
        std::size_t size() const { return m_container.size(); }
        T&          push(T&& t) { return m_container.push_back(std::move(t)); }
        T           pop() { return m_container.pop_front(); }
        T&          front() { return m_container.front(); }
        const T&    front() const { return m_container.front(); }
        T&          back() { return m_container.back(); }
        const T&    back() const { return m_container.back(); }

        Container&       underlying() { return m_container; }
        const Container& underlying() const { return m_container; }

    private:
        Container m_container;
    };
}
