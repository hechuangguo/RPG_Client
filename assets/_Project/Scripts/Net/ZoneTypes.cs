/// <summary>
/// 区服列表条目（对标 net/ZoneTypes.h GameZoneEntry）。
/// </summary>
namespace Rpg.Client.Net
{
    public enum ZoneLoadStatus
    {
        Smooth = 0,
        Busy = 1,
        Full = 2,
        Maintenance = 3
    }

    public sealed class GameZoneEntry
    {
        public uint ZoneId;
        public byte GameType;
        public bool Enabled;
        public string Name = string.Empty;
        public string Ip = string.Empty;
        public ushort SuperPort;
        public uint OnlineCount;
        public ZoneLoadStatus LoadStatus;
        public uint GatewayCount;
    }
}
