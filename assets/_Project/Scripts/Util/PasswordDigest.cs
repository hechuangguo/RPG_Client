/// <summary>
/// 登录 wire 密码摘要 SHA-256(UTF-8)。
/// </summary>
using System.Security.Cryptography;
using System.Text;

namespace Rpg.Client.Util
{
    public static class PasswordDigest
    {
        /// <summary>返回 32 字节 SHA-256 摘要。</summary>
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
