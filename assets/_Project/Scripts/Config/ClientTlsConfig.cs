/// <summary>
/// TLS 传输层配置。
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
