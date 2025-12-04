//
// Created by AWAY on 25-10-14.
//

#pragma once
#include <iostream>
#include <string>
#include <set>
#include "errc.h"
#include "leptjson.h"
#include "types.h"
#include "session.h"
#include <tuple>
#include <type_traits>
#include <functional>
#include <utility>

namespace uno
{

    class Router
    {
    public:
        using arg_t = std::vector<lept_value>;
    private:

#define FUNCTION_TRAITS(traits, ...) \
    template <typename T> \
    struct traits##_helper; \
        \
    template <typename C, typename... Args> \
    struct traits##_helper<void(C::*)(__VA_ARGS__ Args...)> \
    { \
        using args_tuple = std::tuple<Args...>; \
    }; \
        \
    template <typename C, typename... Args> \
    struct traits##_helper<void(C::*)(__VA_ARGS__ Args...) const> \
    { \
        using args_tuple = std::tuple<Args...>; \
    }; \
        \
    template <typename... Args> \
    struct traits##_helper<void(*)(__VA_ARGS__ Args...)> \
    { \
        using args_tuple = std::tuple<Args...>; \
    }; \
        \
    template <typename T, typename = void> \
    struct traits : traits##_helper<T> {}; \
        \
    /* lambda 特例化 */  \
    template <typename Lambda> \
    struct traits<Lambda, std::void_t<decltype(&Lambda::operator())>> : traits##_helper<decltype(&Lambda::operator())> {};


        FUNCTION_TRAITS(function_traits_s2s)
        FUNCTION_TRAITS(function_traits_c2s, SessionPtr,)

#undef FUNCTION_TRAITS

        template<typename Tuple, size_t... Is>
        static Tuple json_to_tuple_impl(const arg_t& args, std::index_sequence<Is...>)
        {
            if (args.size() != sizeof...(Is))
            {
                throw std::runtime_error("Arguments count not match " +
                    std::to_string(args.size()) + " != " +
                    std::to_string(sizeof...(Is)));
            }

            return std::make_tuple(
                args[Is].get<std::tuple_element_t<Is, Tuple>>()...
            );
        }

        template<typename Tuple>
        static Tuple json_to_tuple(const arg_t& args)
        {
            return json_to_tuple_impl<Tuple>(
                args,
                std::make_index_sequence<std::tuple_size_v<Tuple>>{}
            );
        }

    public:

        struct Registerer
        {
#define REG_TAG(name) \
    struct Tag_##name {}; \
    static constexpr Tag_##name name{}; \
        \
    template<typename Func>  \
    Registerer(Tag_##name tag, const std::string& funcname, Func func)  \
    { \
        router().register_##name(funcname, func);  \
    }

            REG_TAG(s2s);
            REG_TAG(c2s);

#undef REG_TAG

        };



        static Router& router()
        {
            static Router instance;
            return instance;
        }

        void call_s2s(const std::string& funcname, const arg_t& args)
        {
            if (!s2s_funcs.contains(funcname))
            {
                std::cerr << "Unhandled internal s2s msg " << funcname << std::endl;
                return ;
            }

            try
            {
                s2s_funcs[funcname](args);
            } catch (const std::exception& e)
            {
                std::cerr << "Error while dispatch s2s msg: " << funcname << std::endl;
                std::cerr << e.what() << std::endl;
            }
        }

        template<typename Func>
        void register_s2s(const std::string& funcname, Func func)
        {
            using Traits = function_traits_s2s<Func>;
            using ArgsTuple = typename Traits::args_tuple;

            s2s_funcs[funcname] = [func](const arg_t& args)
            {
                auto args_tuple = json_to_tuple<ArgsTuple>(args);

                std::apply(func, args_tuple);
            };
        }

        void call_c2s(const std::string& funcname, SessionPtr session, const arg_t& args)
        {
            if (!c2s_funcs.contains(funcname))
            {
                std::cerr << "Unhandled c2s msg " << funcname << std::endl;
                return ;
            }

            try
            {
                c2s_funcs[funcname](std::move(session), args);
            } catch (const std::exception& e)
            {
                std::cerr << "Error while dispatch c2s msg: " << funcname << std::endl;
                std::cerr << e.what() << std::endl;
            }
        }

        template<typename Func>
        void register_c2s(const std::string& funcname, Func func)
        {
            using Traits = function_traits_c2s<Func>;
            using ArgsTuple = typename Traits::args_tuple;

            c2s_funcs[funcname] = [func](SessionPtr ss, const arg_t& args)
            {
                auto args_tuple = json_to_tuple<ArgsTuple>(args);
                auto full_tuple = std::tuple_cat(std::forward_as_tuple(ss), args_tuple);
                std::apply(func, full_tuple);
            };
        }


        bool contain_s2s(const std::string& funcname)
        {
            return s2s_funcs.contains(funcname);
        }

        bool contain_c2s(const std::string& funcname)
        {
            return c2s_funcs.contains(funcname);
        }


    private:
        std::map<std::string, std::function<void(const arg_t&)>> s2s_funcs;
        std::map<std::string, std::function<void(SessionPtr, const arg_t&)>> c2s_funcs;

        Router() = default;
        ~Router() = default;
    };



#define CALL_S2S(funcname, ...) do { \
        Router::router().call_s2s(funcname, {__VA_ARGS__}); \
    } while(0)

#define CALL_C2S(funcname, session, ...) do {\
        Router::router().call_c2s(funcname, session, {__VA_ARGS__}); \
    } while(0)

#define REGISTER_S2S(funcname, func) \
        Router::Registerer(Router::Tag_s2s, funcname, func); \

#define REGISTER_C2S(funcname, func) \
        Router::Registerer(Router::Tag_c2s, funcname, func); \


    // todo: evtcenter


}

