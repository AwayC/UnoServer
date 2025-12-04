//
// Created by AWAY on 25-12-4.
//
#include <functional>
#include <type_traits>

namespace uno
{
    template <typename T> // lambda 特化
    struct function_traits : function_traits<decltype(&T::operator())> {};

    template <typename R, typename... Args>
    struct function_traits<R(*)(Args...)> {
        using args_tuple = std::tuple<Args...>;
        using function_type = std::function<void(Args...)>;
    };

    template <typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...)> {
        using args_tuple = std::tuple<Args...>;
        using function_type = std::function<void(Args...)>;
    };

    template <typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...) const> {
        using args_tuple = std::tuple<Args...>;
        using function_type = std::function<void(Args...)>;
    };

    template <typename T>
    using FunctionType = typename function_traits<std::decay_t<T>>::function_type;
}