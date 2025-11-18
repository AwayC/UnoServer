//
// Created by AWAY on 25-11-17.
//
#include "core/router.h"
#include "core/session.h"
#include "httpserver.h"

using namespace uno;

static int test_count = 0;
static int passed_count = 0;

#define REGISTER_S2S(func) Router::router().register_s2s(#func, S2S::func)
#define REGISTER_C2S(func) Router::router().register_c2s(#func, C2S::func)

namespace S2S
{
    void sync_void1()
    {
        std::cout << "S2S sync_void1" << std::endl;
        ++passed_count;
    }

    void sync_void2()
    {
        std::cout << "S2S sync_void2" << std::endl;
        ++passed_count;
    }

    void sync_int(int a)
    {
        std::cout << "S2S sync_int: " << a << std::endl;
        ++passed_count;
    }

    void sync_double(double a)
    {
        std::cout << "S2S sync_double: " << a << std::endl;
        ++passed_count;
    }



    void sync_bool(bool a)
    {
        std::cout << "S2S sync_bool: " << a << std::endl;
        ++passed_count;
    }

    void sync_string(std::string a)
    {
        std::cout << "S2S sync_string: " << a << std::endl;
        ++passed_count;
    }

    void sync_array(lept_value::array_t a)
    {
        std::cout << "S2S sync_array: " << a.size() << std::endl;
        for (auto i : a)
        {
            std::cout << "S2S sync_array: " << i.stringify() << std::endl;
        }
        ++passed_count;
    }

    void sync_object(lept_value::object_t a)
    {
        std::cout << "S2S sync_object: " << a.size() << std::endl;
        for (auto i : a)
        {
            std::cout << "S2S sync_object: " << i.first << " -> " << i.second.stringify() << std::endl;
        }
        ++passed_count;
    }

    void sync_args(int i, double d, bool b, std::string s, lept_value::array_t a, lept_value::object_t o)
    {
        std::cout << "S2S sync_args: " << i << " " << d << " " << b << " " << s << " " << a.size() << " " << o.size() << std::endl;
        ++passed_count;
    }
}


namespace C2S
{
    void sync_void1(SessionPtr ss)
    {
        std::cout << "C2S sync_void1" << std::endl;
        ++passed_count;
    }

    void sync_void2(SessionPtr ss)
    {
        std::cout << "C2S sync_void2" << std::endl;
        ++passed_count;
    }

    void sync_int(SessionPtr ss, int a)
    {
        std::cout << "C2S sync_int: " << a << std::endl;
        ++passed_count;
    }

    void sync_double(SessionPtr ss, double a)
    {
        std::cout << "C2S sync_double: " << a << std::endl;
        ++passed_count;
    }

    void sync_bool(SessionPtr ss, bool a)
    {
        std::cout << "C2S sync_bool: " << a << std::endl;
        ++passed_count;
    }

    void sync_string(SessionPtr ss, std::string a)
    {
        std::cout << "C2S sync_string: " << a << std::endl;
        ++passed_count;
    }

    void sync_array(SessionPtr ss, lept_value::array_t a)
    {
        std::cout << "C2S sync_array: " << a.size() << std::endl;
        for (auto i : a)
        {
            std::cout << "C2S sync_array: " << i.stringify() << std::endl;
        }
        ++passed_count;
    }

    void sync_object(SessionPtr ss, lept_value::object_t a)
    {
        std::cout << "C2S sync_object: " << a.size() << std::endl;
        for (auto i : a)
        {
            std::cout << "C2S sync_object: " << i.first << " -> " << i.second.stringify() << std::endl;
        }
        ++passed_count;
    }

    void sync_args(SessionPtr ss, int i, double d, bool b, std::string s, lept_value::array_t a, lept_value::object_t o)
    {
        std::cout << "C2S sync_args: " << i << " " << d << " " << b << " " << s << " " << a.size() << " " << o.size() << std::endl;
        ++passed_count;
    }
}

#define TEST_FUNC_S2S(funcname_, ...) do { \
        ++test_count; \
        REGISTER_S2S(funcname_); \
        Router::arg_t args_ = Router::arg_t({__VA_ARGS__}); \
        Router::router().call_s2s(#funcname_, args_); \
    } while (0);

#define TEST_FUNC_C2S(funcname, ...) do { \
        test_count ++; \
        REGISTER_C2S(funcname); \
        auto session_ = Session::create(); \
        Router::arg_t args_ = Router::arg_t({__VA_ARGS__}); \
        Router::router().call_c2s(#funcname, session_, args_); \
    } while(0);


void test_s2s()
{
    TEST_FUNC_S2S(sync_void1);
    TEST_FUNC_S2S(sync_void2);
    TEST_FUNC_S2S(sync_int, lept_value(123));
    TEST_FUNC_S2S(sync_double, lept_value(1.23));
    TEST_FUNC_S2S(sync_bool, lept_value(true));
    TEST_FUNC_S2S(sync_string, "hello");
    TEST_FUNC_S2S(sync_array, {1, 2});
    TEST_FUNC_S2S(sync_object, {{"a", 1}, {"b", 2}});
    TEST_FUNC_S2S(sync_args, 1, 1.23, true, "hello", {"a", 1}, {{"key", "val"}, {"b", 2}});

}

void test_c2s()
{
    TEST_FUNC_C2S(sync_void1);
    TEST_FUNC_C2S(sync_void2);
    TEST_FUNC_C2S(sync_int, lept_value(123));
    TEST_FUNC_C2S(sync_double, lept_value(1.23));
    TEST_FUNC_C2S(sync_bool, lept_value(true));
    TEST_FUNC_C2S(sync_string, "hello");
    TEST_FUNC_C2S(sync_array, {1, 2});
    TEST_FUNC_C2S(sync_object, {{"a", 1}, {"b", 2}});
    TEST_FUNC_C2S(sync_args, 1, 1.23, true, "hello", {"a", 1}, {{"key", "val"}, {"b", 2}});
}

int main() {
    test_s2s();
    test_c2s();

    printf("%d/%d (%.2f%%) passed\n", passed_count, test_count, (double)passed_count * 100 / test_count);

    if (passed_count == test_count)
    {
        std::cout << "All tests passed" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "Some tests failed" << std::endl;
        return 1;
    }
}