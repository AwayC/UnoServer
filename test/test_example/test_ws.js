const axios = require('axios');
const md5 = require('blueimp-md5');
const WebSocket = require('ws');

// --- 配置区域 ---
const HTTP_BASE_URL = 'http://localhost:8081';
const WS_URL = 'ws://localhost:8081/ws';

// 使用一个新账号以确保测试 "创建角色" 流程
const TIMESTAMP = Math.floor(Date.now() / 1000);
const TEST_USER = {
    name: `player_2`,
    password: 'password123',
    email: `player_2@example.com`
};

// 模拟角色信息
const ROLE_INFO = {
    name: `Hero_02`,
};

let token = '';

const axiosConfig = { proxy: false };

async function startTest() {
    console.log(`========== 开始游戏业务流程测试 ==========`);
    console.log(`测试账号: ${TEST_USER.name}`);

    // -------------------------------------------------
    // 1. HTTP 注册 & 登录 (获取 Token)
    // -------------------------------------------------
    try {
        // 注册
        await axios.post(`${HTTP_BASE_URL}/api/register`, {
            name: TEST_USER.name,
            password: md5(TEST_USER.password),
            email: TEST_USER.email
        }, axiosConfig);

        // 登录
        const loginRes = await axios.post(`${HTTP_BASE_URL}/api/login`, {
            name: TEST_USER.name,
            password: md5(TEST_USER.password)
        }, axiosConfig);

        if (loginRes.data.code === 0) {
            token = loginRes.data.data.token;
            console.log(`✅ [HTTP] 登录成功，Token 获取完成`);
        } else {
            console.error(`❌ [HTTP] 登录失败`, loginRes.data);
            return;
        }
    } catch (e) {
        console.error(`❌ [HTTP] 请求异常: ${e.message}`);
        return;
    }

    // -------------------------------------------------
    // 2. WebSocket 连接
    // -------------------------------------------------
    console.log(`\n>> 正在连接 WebSocket...`);
    const ws = new WebSocket(WS_URL, { perMessageDeflate: false });

    ws.on('open', function () {
        console.log(`✅ [WS] 连接已建立`);

        // --- 步骤 A: 发送账号登录请求 ---
        // 协议: ["login_req", token]
        send(ws, 'login_req', token);
    });

    ws.on('message', function (data) {
        const msgStr = data.toString();
        // console.log(`<< [收到] ${msgStr}`); // 调试用，太长可注释

        try {
            const arr = JSON.parse(msgStr);
            const cmd = arr[0];
            const args = arr.slice(1); // 参数列表

            handleServerMessage(ws, cmd, args);
        } catch (e) {
            console.error(`⚠️ 解析失败: ${e.message}`);
        }
    });

    ws.on('error', (err) => console.error(`❌ [WS] 错误: ${err.message}`));
    ws.on('close', () => console.log(`[WS] 连接断开`));
}

// -------------------------------------------------
// 业务逻辑处理中心
// -------------------------------------------------
function handleServerMessage(ws, cmd, args) {
    console.log(`<< [服务器] ${cmd}:`, JSON.stringify(args).substring(0, 100) + "...");

    switch (cmd) {
        case 'create_role_ntf':
        // args: [ ]
        
            send(ws, 'create_role_req', ROLE_INFO.name);

            break;

        case 'create_role_rsp':
            // args: [errCode, roleData]
            if (args[0] === 0) {
                console.log(`   ✅ 角色创建成功！`);
                
                // --- 步骤 C: 角色登录/进入游戏 ---
                // 有些游戏创建完会自动登录，有些需要手动发 login_role
                // 这里模拟发送角色登录
                console.log(`   >> 发送角色登录/进入游戏请求...`);
                send(ws, 'login_req', token); // 这里的协议名可能需要改为你的实际协议，如 'enter_game_req'
            } else {
                console.error(`   ❌ 角色创建失败: Code ${args[0]} (可能是名字已存在)`);
                // 如果失败，可能是老账号，尝试直接登录角色
                send(ws, 'login_req', token);
            }
            break;

        case 'login_rsp': 
            if (args[0] === 0) {
                console.log(`   ✅ 角色登录成功！进入大厅。`);
            } else {
                console.error(`   ❌ 角色登录失败: Code ${args[0]}`);
            }
            break;

        case 'save_role_rsp':
            if (args[0] === 0) {
                console.log(`   ✅ 角色数据保存成功！`);
                console.log(`\n🎉 所有测试步骤执行完毕！`);
                process.exit(0);
            } else {
                console.error(`   ❌ 保存失败: Code ${args[0]}`);
            }
            break;
            
        default:
            // 忽略心跳等其他消息
            break;
    }
}

// 辅助发送函数
function send(ws, funcName, ...args) {
    const packet = [funcName, ...args];
    const str = JSON.stringify(packet);
    console.log(`>> [发送] ${str}`);
    ws.send(str);
}

// 启动
startTest();