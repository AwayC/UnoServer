//
// Created by AWAY on 25-12-4.
//
#include <core/router.h>
#include "core/session.h"
#include "core/EventEmitter.h"

void login(std::string &name, int id) {
    std::cout << "[Login] User: " << name << ", ID: " << id << std::endl;
}

int main() {
    // 关闭同步
    // std::cout.tie(&std::cerr);

    auto& bus = uno::evtcenter;

    bus.on("login", login);

    auto ss = std::make_shared<uno::Session>();
    bus.on("login", [ss](std::string &name, int id) {
        std::cout << "ss uid: " << ss->uid() << std::endl;
        std::cout << "[Login] Logged to database for ID: " << id << std::endl;
    });

    // 场景 3：注册另一个完全不同的事件 "update"，没有任何参数
    bus.on("update", []() { // void 表示无参
        std::cout << "[Update] System updated." << std::endl;
    });

    std::cout << "--- 1. 正常触发 Login ---" << std::endl;
    // 正确：参数匹配 (string&, int)
    // 注意：不要显式指定 template<std::string, int>，否则会变为值传递，导致与 registered 的 (string&, int) 不匹配
    // 且 explicit args 会导致 forwarding references 失效 (变为 rvalue ref)，导致 lvalue 无法传递
    std::string name = "Alice";
    bus.emit("login", name, 1001);

    std::cout << "\n--- 2. 正常触发 Update ---" << std::endl;
    bus.emit("update");

    std::cout << "\n--- 2.1. 使用 emit_as 显式指定签名 (针对 const/ref 不匹配的情况) ---" << std::endl;
    // 如果 deduction 出来的类型不匹配 (例如传递 string 到 const string&)，可以使用 emit_as
    bus.emit_as<void(std::string&, int)>("login", name, 1002);

    std::cout << "\n--- 3. 错误触发 Login (参数类型错误) ---" << std::endl;
    bus.emit("login", 1001, name);

    std::cout << "\n--- 4. 错误触发 Login (参数数量错误) ---" << std::endl;
    bus.emit<std::string>("login", "Bob");

    return 0;
}