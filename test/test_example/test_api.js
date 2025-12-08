const axios = require('axios');
const md5 = require('blueimp-md5');

// 配置：根据你的实际环境修改
const BASE_URL = 'http://localhost:8081';

// 测试用的账号信息
const TEST_USER = {
    name: 'testuser_003',
    password: 'password123',
    email: 'test@example.com'
};

const NEW_INFO = {
    email: 'new_email@example.com',
    password: 'newpassword456'
};

async function testHttpApi() {
    console.log(`[开始测试] 目标服务器: ${BASE_URL}`);
    console.log(`[测试数据] 用户: ${TEST_USER.name}`);

    // 用于禁用代理的配置（防止 127.0.0.1 被代理拦截）
    const axiosConfig = { proxy: false };

    let token = '';

    try {
        // --- 1. 尝试注册 ---
        console.log('\n--- 1. 测试注册接口 (/api/register) ---');
        try {
            const regRes = await axios.post(`${BASE_URL}/api/register`, {
                name: TEST_USER.name,
                password: md5(TEST_USER.password),
                email: TEST_USER.email
            }, axiosConfig);
            console.log('注册响应:', regRes.data);
        } catch (regErr) {
            if (regErr.response) {
                console.log('注册返回 (可能是用户已存在):', regErr.response.data);
            } else {
                console.error('注册请求失败:', regErr.message);
            }
        }

        // --- 2. 尝试登录 ---
        console.log('\n--- 2. 测试登录接口 (/api/login) ---');
        const loginRes = await axios.post(`${BASE_URL}/api/login`, {
            name: TEST_USER.name,
            password: md5(TEST_USER.password)
        }, axiosConfig);

        console.log('登录响应:', loginRes.data);

        if (loginRes.data.code === 0) {
            token = loginRes.data.data.token;
            console.log('>>> 登录成功！获取到 Token:', token);
        } else {
            console.error('>>> 登录失败，无法继续测试后续步骤。业务代码:', loginRes.data.code);
            return;
        }

        // --- 3. 更新 Email ---
        // 参考 LobbyPanel.vue: /api/update_email, 参数: { token, email }
        console.log("\n--- 3. 测试更新邮箱接口 (/api/update_email) ---");
        const updateEmailRes = await axios.post(`${BASE_URL}/api/update_email`, {
            token: token,
            email: NEW_INFO.email
        }, axiosConfig);
        console.log('更新邮箱响应:', updateEmailRes.data);

        if (updateEmailRes.data.code === 0) {
            console.log(`>>> 邮箱更新成功！新邮箱: ${NEW_INFO.email}`);
        } else {
            console.error('>>> 邮箱更新失败，业务代码:', updateEmailRes.data.code);
        }

        // --- 4. 更新密码 ---
        // 参考 LobbyPanel.vue: /api/update_password, 参数: { token, password: md5(new_pass) }
        console.log("\n--- 4. 测试更新密码接口 (/api/update_password) ---");
        const updatePasswordRes = await axios.post(`${BASE_URL}/api/update_password`, {
            token: token,
            password: md5(NEW_INFO.password) // 注意：这里通常也需要发 MD5
        }, axiosConfig);
        console.log('更新密码响应:', updatePasswordRes.data);

        if (updatePasswordRes.data.code === 0) {
            console.log('>>> 密码更新成功！请尝试使用新密码登录验证。');
        } else {
            console.error('>>> 密码更新失败，业务代码:', updatePasswordRes.data.code);
        }

        // --- 5. (可选) 使用新密码验证登录 ---
        console.log("\n--- 5. 验证新密码登录 ---");
        const reLoginRes = await axios.post(`${BASE_URL}/api/login`, {
            name: TEST_USER.name,
            password: md5(NEW_INFO.password)
        }, axiosConfig);

        if (reLoginRes.data.code === 0) {
            console.log('>>> 验证成功！新密码可用。');
        } else {
            console.log('>>> 验证失败！新密码无法登录。');
        }

    } catch (err) {
        console.error('\n[致命错误] HTTP 连接或请求失败:');
        if (err.code === 'ECONNREFUSED') {
            console.error('无法连接到服务器。请检查后端是否启动及端口配置。');
        } else {
            console.error(err.message);
        }
    }
}

// 执行测试
testHttpApi();