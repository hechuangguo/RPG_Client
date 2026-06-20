/**
 * @file    PasswordDigest.cpp
 * @brief   PasswordDigest 实现
 */

#include "util/PasswordDigest.h"

#include "log/ClientLogger.h"

#include <openssl/evp.h>

namespace PasswordDigest
{
bool sha256Utf8Password(const std::string& password, uint8_t digest[32])
{
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        ClientLogger::instance().warn("PasswordDigest：EVP_MD_CTX_new 失败");
        return false;
    }

    bool ok = false;
    unsigned int outLen = 0;
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
        EVP_DigestUpdate(ctx, password.data(), password.size()) == 1 &&
        EVP_DigestFinal_ex(ctx, digest, &outLen) == 1 &&
        outLen == 32)
    {
        ok = true;
    }
    else
    {
        ClientLogger::instance().warn("PasswordDigest：SHA-256 计算失败");
    }

    EVP_MD_CTX_free(ctx);
    return ok;
}
}  // namespace PasswordDigest
