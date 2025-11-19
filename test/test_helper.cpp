//
// Created by AWAY on 25-11-19.
//

#include <iostream>

#include "game/helper.h"
#include "leptjson.h"

using namespace uno;

void test_random_select()
{
    std::vector<int> int_arr = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    for (int i = 0;i < 10;i ++)
    {
        std::cout << "test int arr: " << helper::random_select(int_arr) << std::endl;
    }

    std::vector<lept_value> json_arr = {123, nullptr, 1.5, "hello", true, false,
                {"arr", 342, false},
                {{"key", "value"}, {"null", nullptr}}};

    for (int i = 0;i < 10;i ++)
    {
        std::cout << "test lept json arr: " << helper::random_select(json_arr).stringify() << std::endl;
    }
}

void test_random_shuffle()
{
    std::vector<int> int_arr = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    helper::random_shuffle(int_arr);

    for (auto i : int_arr)
    {
        std::cout << int_arr[i] <<  ", ";
    }
    std::cout << std::endl;

    std::vector<lept_value> json_arr = {123, nullptr, 1.5, "hello", true, false,
                {"arr", 342, false},
                {{"key", "value"}, {"null", nullptr}}};
    helper::random_shuffle(json_arr);

    for (auto i : json_arr)
    {
        std::cout << i.stringify() << ", ";
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    test_random_select();
    test_random_shuffle();
}