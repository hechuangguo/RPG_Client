/// <summary>
/// 网络时序默认值（对标 sdk/net/ClientTimingDefaults.h）。
/// client_config.xml 缺失项时回退到此。
/// </summary>
namespace Rpg.Client.Config
{
    public static class ClientTimingDefaults
    {
        public const long ConnectTimeoutMs = 10_000;
        public const long ResponseTimeoutMs = 15_000;
        public const long ZoneListResponseTimeoutMs = 10_000;
        public const long HeartbeatIntervalMs = 10_000;
        public const long MoveSendIntervalMs = 100;
        public const long LogoutTimeoutMs = 15_000;
    }
}
