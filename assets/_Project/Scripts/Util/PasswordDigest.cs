/// <summary>
/// 登录 Protobuf 密码摘要：password_digest = SHA-256(UTF-8 密码)；login_nonce 单独回显挑战值。
/// </summary>
using System.Security.Cryptography;
using System.Text;

namespace Rpg.Client.Util
{
    public static class PasswordDigest
    {
        public const int DigestLength = 32;

        /// <summary>返回 32 字节 SHA-256(UTF-8 密码)，用于 C2SLoginReq / C2SRegisterReq 的 password_digest。</summary>
        public static byte[] Sha256Utf8Password(string password)
        {
            var bytes = Encoding.UTF8.GetBytes(password ?? string.Empty);
#if NET5_0_OR_GREATER
            return SHA256.HashData(bytes);
#else
            using var sha = SHA256.Create();
            return sha.ComputeHash(bytes);
#endif
        }
    }
}
