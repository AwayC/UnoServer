//
// Created by AWAY on 25-11-23.
//

#pragma once

#include <iomanip>
#include <string>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/rand.h>

namespace uno {
    inline std::string base64_encode(const unsigned char* input, int length) {
        int encoded_len = 4 * ((length + 2) / 3);

        std::vector<unsigned char> buffer(encoded_len + 1);

        EVP_EncodeBlock(buffer.data(), input, length);

        return std::string(reinterpret_cast<char*>(buffer.data()));
    }

    inline std::string generate_salt() {
        unsigned char random_bytes[8];

        if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
            return "";
        }

        return base64_encode(random_bytes, sizeof(random_bytes));
    }

    inline std::string md5(const std::string& input)
    {
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) return "";

        // 初始化 MD5
        if (EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) != 1) {
            EVP_MD_CTX_free(ctx);
            return "";
        }

        // 喂数据
        if (EVP_DigestUpdate(ctx, input.c_str(), input.length()) != 1) {
            EVP_MD_CTX_free(ctx);
            return "";
        }

        // 取结果
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int digest_len = 0;
        if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
            EVP_MD_CTX_free(ctx);
            return "";
        }

        EVP_MD_CTX_free(ctx);

        // 转 32位 小写 Hex 字符串
        std::stringstream ss;
        for(unsigned int i = 0; i < digest_len; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
        }
        return ss.str();
    }


};