//
// Created by AWAY on 25-11-19.
//

#pragma once
#include <vector>
#include <random>
#include <cassert>
#include <algorithm>

namespace uno
{
    namespace helper
    {
        inline std::mt19937& get_random_engine() {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            return gen;
        }

        template<typename T>
        const T& random_select(const std::vector<T>& arr) {
            assert(arr.size());

            std::mt19937& engine = get_random_engine();
            std::uniform_int_distribution<size_t> distrib(0, arr.size() - 1);

            size_t random_index = distrib(engine);
            return arr[random_index];
        }

        template<typename T>
        std::vector<T>& random_shuffle(std::vector<T>& arr) {
            std::shuffle(arr.begin(), arr.end(), get_random_engine());
            return arr;
        }
    }
}