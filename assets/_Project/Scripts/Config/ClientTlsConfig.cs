/// <summary>
/// TLS 传输层配置。
/// 生产环境：InsecureSkipVerify = false（默认）。
/// 本地开发/自签证书环境：可手动设置为 true，但绝不应出现在发布包中。
/// </summary>
namespace Rpg.Client.Config
{
    public sealed class ClientTlsConfig
    {
        public bool Enabled { get; set; } = true;
        public string CaPath { get; set; } = "config/tls/ca.crt";

        /// <summary>
        /// 是否跳过 TLS 证书校验。
        /// 默认 false（生产安全）。
        /// 仅在本地开发/自签证书调试时设为 true，切勿进入正式包。
        /// </summary>
        public bool InsecureSkipVerify { get; set; } = false;

        public string MinVersion { get; set; } = "1.2";
    }
}
