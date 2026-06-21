/// <summary>
/// TLS 传输层配置（对标 sdk/net/ClientTlsConfig.h）。
/// 从 client_config.xml 的 Tls 段解析。
/// </summary>
namespace Rpg.Client.Config
{
    public sealed class ClientTlsConfig
    {
        public bool Enabled { get; set; } = true;
        public string CaPath { get; set; } = "config/tls/ca.crt";
        public bool InsecureSkipVerify { get; set; } = true;
        public string MinVersion { get; set; } = "1.2";
    }
}
