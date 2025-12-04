//
// Created by AWAY on 25-12-4.
//
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <any>
#include <stdexcept>
#include "func_trait.h"

class EventEmitter {
private:
    // Key: 事件名
    // Value: 回调函数列表
    std::map<std::string, std::vector<std::any>> listeners;

public:
    template <typename Func>
    void on(const std::string& eventName, Func&& callback) {
        listeners[eventName].push_back(FunctionType<Func>(std::forward<Func>(callback)));
    }


    template <typename... Args>
    void emit(const std::string& eventName, Args... args) {
        if (listeners.find(eventName) == listeners.end()) {
            return;
        }

        auto& callbacks = listeners[eventName];
        for (auto& cbAny : callbacks) {
            try {
                auto fn = std::any_cast<std::function<void(Args...)>>(cbAny);
                fn(args...);
            } catch (const std::bad_any_cast& e) {
                std::cerr << "Error: 事件 '" << eventName
                          << "' 的回调签名不匹配！emit 参数与 on 参数不一致。" << std::endl;
            }
        }
    }
};
