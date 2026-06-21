/// <summary>
/// 时间工具（对标 sdk/time/TimeUtil）。
/// </summary>
namespace Rpg.Client.Util
{
    public static class TimeUtil
    {
        public static long NowMs() => (long)(UnityEngine.Time.realtimeSinceStartupAsDouble * 1000.0);

        public static long WallNowMs() =>
            (long)System.DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
    }
}
