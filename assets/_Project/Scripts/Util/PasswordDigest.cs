/// <summary>
/// 登录 Protobuf 密码摘要：SHA-256(nonce || UTF-8 密码)。
/// </summary>
using System;
using System.Security.Cryptography;
using System.Text;

namespace Rpg.Client.Util
{
    public static class PasswordDigest
    {
        public const int NonceLength = 16;
        public const int DigestLength = 32;

        /// <summary>返回 32 字节 SHA-256(nonce || UTF-8 密码)。</summary>
        public static byte[] Sha256NoncePassword(byte[] nonce, string password)
        {
            if (nonce == null || nonce.Length == 0)
            {
                throw new ArgumentException("登录 nonce 不能为空", nameof(nonce));
            }

            var pwdBytes = Encoding.UTF8.GetBytes(password ?? string.Empty);
            var combined = new byte[nonce.Length + pwdBytes.Length];
            Buffer.BlockCopy(nonce, 0, combined, 0, nonce.Length);
            Buffer.BlockCopy(pwdBytes, 0, combined, nonce.Length, pwdBytes.Length);
#if NET5_0_OR_GREATER
            return SHA256.HashData(combined);
#else
            using var sha = SHA256.Create();
            return sha.ComputeHash(combined);
#endif
        }
    }
}
