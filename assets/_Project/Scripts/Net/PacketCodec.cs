/// <summary>
/// 协议封包与拆包（对标 sdk/net/ProtocolCodec.h）。
/// 职责：MsgHeader + Protobuf body 编解码；不含 TLS。
/// 协作：TcpClient、LoginSession、GameSession。
/// 线程：Unity 主线程。
/// </summary>
using System;
using System.Collections.Generic;
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

        /// <summary>从接收缓冲尝试拆出一帧；成功时移除已消费字节。</summary>
        public static bool TryDecode(
            List<byte> buffer,
            out byte module,
            out byte sub,
            out byte[] body)
        {
            module = 0;
            sub = 0;
            body = Array.Empty<byte>();

            if (buffer.Count < MsgHeader.Size)
            {
                return false;
            }

            if (!MsgHeader.TryRead(buffer.ToArray().AsSpan(0, MsgHeader.Size), out var header))
            {
                return false;
            }

            var total = MsgHeader.Size + header.BodyLen;
            if (buffer.Count < total)
            {
                return false;
            }

            if (header.BodyLen > 0)
            {
                body = new byte[header.BodyLen];
                Buffer.BlockCopy(buffer.ToArray(), MsgHeader.Size, body, 0, header.BodyLen);
            }

            module = header.Module;
            sub = header.Sub;
            buffer.RemoveRange(0, total);
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
