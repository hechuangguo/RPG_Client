/// <summary>
/// 网络时序默认值。
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
        public const int MaxFlushBytesPerPoll = 16_384;
        public const float MoveMinDelta = 0.01f;
    }
}

