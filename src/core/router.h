//
// Created by AWAY on 25-10-14.
//

#pragma once
#include <iostream>
#include <string>

namespace uno
{
    class Router {
    public:
        Router() = default;
        ~Router() = default;

        template<typename... Args>
        void call(const std::string& funcname, Args&&... args)
        {
            std::cout << "call function: " << funcname << std::endl;
            try
            {
                if (funcname == "add")
                {
                    // todo function apply
                }
                else
                {
                    std::cerr << "unknown function: " << funcname << std::endl;
                }
            } catch (const std::exception& e)
            {
                std::cerr << "Err while dispatch s2s msg: " << e.what() << std::endl;
            }


        }
    };
}




