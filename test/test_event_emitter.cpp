//
// Created by AWAY on 25-12-4.
//
#include <core/router.h>
#include "core/session.h"
#include "core/EventEmitter.h"

void login(std::string name, int id) {
    std::cout << "[Login] User: " << name << ", ID: " << id << std::endl;
}

int main() {
    auto& bus = uno::evtcenter;

    bus.on("login", login);

    auto ss = std::make_shared<uno::Session>();
    bus.on("login", [ss](std::string name, int id) {
        std::cout << "ss uid: " << ss->uid() << std::endl;
        std::cout << "[Login] Logged to database for ID: " << id << std::endl;
    });

    // 场景 3：注册另一个完全不同的事件 "update"，没有任何参数
    bus.on("update", []() { // void 表示无参
        std::cout << "[Update] System updated." << std::endl;
    });

    std::cout << "--- 1. 正常触发 Login ---" << std::endl;
    // 正确：参数匹配 (string, int)
    bus.emit<std::string, int>("login", "Alice", 1001);

    std::cout << "\n--- 2. 正常触发 Update ---" << std::endl;
    bus.emit("update");

    std::cout << "\n--- 3. 错误触发 Login (参数类型错误) ---" << std::endl;
    bus.emit("login", 1001, "Alice");

    std::cout << "\n--- 4. 错误触发 Login (参数数量错误) ---" << std::endl;
    bus.emit<std::string>("login", "Bob");

    return 0;
}