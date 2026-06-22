/// <summary>
/// 协议封包与拆包：4 字节定长 MsgHeader + Protobuf body（payload 不含 module/sub 前缀）。
/// </summary>
using System;
using Google.Protobuf;

namespace Rpg.Client.Net
{
    public static class PacketCodec
    {
        /// <summary>编码 IMessage 为完整帧字节。</summary>
        public static byte[] Encode(byte module, byte sub, IMessage body)
        {
            var payload = body?.ToByteArray() ?? Array.Empty<byte>();
            if (payload.Length > MsgHeader.MaxPacketSize)
            {
                throw new InvalidOperationException(
                    $"Protobuf body 过长：{payload.Length} > {MsgHeader.MaxPacketSize}");
            }

            var frame = new byte[MsgHeader.Size + payload.Length];
            var header = new MsgHeader((ushort)payload.Length, module, sub);
            header.WriteTo(frame.AsSpan(0, MsgHeader.Size));
            if (payload.Length > 0)
            {
                Buffer.BlockCopy(payload, 0, frame, MsgHeader.Size, payload.Length);
            }

            return frame;
        }

        /// <summary>从接收缓冲尝试拆出一帧；成功时移除已消费字节。protocolError 表示帧头非法须断连。</summary>
        public static bool TryDecode(
            byte[] buffer,
            ref int length,
            out byte module,
            out byte sub,
            out byte[] body,
            out bool protocolError)
        {
            module = 0;
            sub = 0;
            body = Array.Empty<byte>();
            protocolError = false;

            if (length < MsgHeader.Size)
            {
                return false;
            }

            if (!MsgHeader.TryRead(buffer.AsSpan(0, length), out var header))
            {
                protocolError = true;
                return false;
            }

            if (header.BodyLen > MsgHeader.MaxPacketSize)
            {
                protocolError = true;
                return false;
            }

            var total = MsgHeader.Size + header.BodyLen;
            if (length < total)
            {
                return false;
            }

            if (header.BodyLen > 0)
            {
                body = new byte[header.BodyLen];
                Buffer.BlockCopy(buffer, MsgHeader.Size, body, 0, header.BodyLen);
            }

            module = header.Module;
            sub = header.Sub;

            var remaining = length - total;
            if (remaining > 0)
            {
                Buffer.BlockCopy(buffer, total, buffer, 0, remaining);
            }

            length = remaining;
            return true;
        }

        /// <summary>按 module/sub 解析 Protobuf 消息。</summary>
        public static bool TryParse<T>(byte module, byte sub, byte expectedModule, byte expectedSub,
            ReadOnlySpan<byte> body, MessageParser<T> parser, out T message)
            where T : IMessage<T>
        {
            message = default;
            if (module != expectedModule || sub != expectedSub)
            {
                return false;
            }

            message = parser.ParseFrom(body);
            return message != null;
        }
    }
}
