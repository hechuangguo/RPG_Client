/// <summary>
/// 6 字节定长帧头（对标 Common/NetDefine.h MsgHeader）。
/// 实际 wire 为 4 字节：bodyLen(2) + module(1) + sub(1)，#pragma pack(1)。
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

        /// <summary>写入小端 6 字节。</summary>
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
