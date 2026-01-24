//
// Created by AWAY on 25-12-4.
//
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <any>
#include <stdexcept>
#include "func_trait.h"

namespace uno
{

    class EventEmitter {
    private:
        // Key: 事件名
        // Value: 回调函数列表
        std::unordered_map<std::string, std::vector<std::any>> listeners;

    public:
        /**
         * 注册函数参数支持引用和const
         * 注意：emit时参数类型必须严格匹配，或使用 emit_as 指定目标签名
         */
        template <typename Func>
        void on(const std::string& eventName, Func&& callback) {
            listeners[eventName].push_back(FunctionType<Func>(std::forward<Func>(callback)));
        }

        template <typename... Args>
        void emit(const std::string& eventName, Args&&... args) {
            emit_as<void(Args...)>(eventName, std::forward<Args>(args)...);
        }

        template <typename Signature, typename... Args>
        void emit_as(const std::string& eventName, Args&&... args) {
            if (listeners.find(eventName) == listeners.end()) {
                return;
            }

            auto& callbacks = listeners[eventName];
            for (auto& cbAny : callbacks) {
                try {
                    auto fn = std::any_cast<std::function<Signature>>(cbAny);
                    fn(std::forward<Args>(args)...);
                } catch (const std::bad_any_cast& e) {
                    std::cerr << "Error: 事件 '" << eventName
                              << "' 的回调签名不匹配！emit 类型与 on 参数不一致。" << std::endl;
                }
            }
        }
    };

}
