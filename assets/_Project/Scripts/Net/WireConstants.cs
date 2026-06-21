/// <summary>
/// Protobuf 协议常量（协议版本等）。
/// </summary>
using Rpg.Proto.Client;

namespace Rpg.Client.Net
{
    public static class WireConstants
    {
        public const byte ZoneListAllGameTypes = 0xFF;

        public static ProtocolVersion CurrentProtocolVersion => new ProtocolVersion
        {
            Major = 1,
            Minor = 0
        };
    }
}
