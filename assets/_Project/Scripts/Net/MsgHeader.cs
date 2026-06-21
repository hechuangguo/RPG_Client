/// <summary>
/// 4 字节定长帧头（与 Common/WireCommon.proto WireMsgHeader 一致）。
/// wire：bodyLen(2) + module(1) + sub(1)，小端 packed。
/// </summary>
using System;
using System.Buffers.Binary;

namespace Rpg.Client.Net
{
    public readonly struct MsgHeader
    {
        public const int Size = 4;
        public const int MaxPacketSize = 65535;

        public ushort BodyLen { get; }
        public byte Module { get; }
        public byte Sub { get; }

        public MsgHeader(ushort bodyLen, byte module, byte sub)
        {
            BodyLen = bodyLen;
            Module = module;
            Sub = sub;
        }

        /// <summary>写入小端 4 字节。</summary>
        public void WriteTo(Span<byte> dest)
        {
            if (dest.Length < Size)
            {
                throw new ArgumentException("buffer too small for MsgHeader");
            }

            BinaryPrimitives.WriteUInt16LittleEndian(dest, BodyLen);
            dest[2] = Module;
            dest[3] = Sub;
        }

        /// <summary>从缓冲读取帧头；数据不足返回 false。</summary>
        public static bool TryRead(ReadOnlySpan<byte> buffer, out MsgHeader header)
        {
            header = default;
            if (buffer.Length < Size)
            {
                return false;
            }

            var bodyLen = BinaryPrimitives.ReadUInt16LittleEndian(buffer);
            header = new MsgHeader(bodyLen, buffer[2], buffer[3]);
            return true;
        }

        public static ushort MakeFlatId(byte module, byte sub) =>
            (ushort)((module << 8) | sub);
    }
}
