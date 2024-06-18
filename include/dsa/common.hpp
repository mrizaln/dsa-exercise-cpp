#pragma once

#include <concepts>

namespace dsa
{
    template <typename T, typename R = std::size_t>
    concept HasSizeMethod = requires(T t) {
        { t.size() } -> std::same_as<R>;
    };
}
