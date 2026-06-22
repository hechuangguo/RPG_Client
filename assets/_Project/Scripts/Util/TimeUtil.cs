/// <summary>
/// 时间工具：Unity 单调启动时钟（毫秒），用于超时与节流。
/// </summary>
namespace Rpg.Client.Util
{
    public static class TimeUtil
    {
        /// <summary>自启动以来的单调毫秒，适合超时/节流；勿用于跨会话持久化。</summary>
        public static long NowMs() => (long)(UnityEngine.Time.realtimeSinceStartupAsDouble * 1000.0);

        /// <summary>UTC 墙钟毫秒；仅用于需与服务器墙钟对齐的场景。</summary>
        public static long WallNowMs() =>
            (long)System.DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();

        /// <summary>安全计算经过毫秒；时钟回拨时返回 0 以便超时逻辑仍可触发。</summary>
        public static long ElapsedMs(long nowMs, long startMs)
        {
            var elapsed = nowMs - startMs;
            return elapsed < 0 ? 0 : elapsed;
        }
    }
}
