/// <summary>
/// Protobuf body 解析 helper；供 ClientMsgHandler 语义化 TryParse 包装复用。
/// </summary>
using Google.Protobuf;

namespace Rpg.Client.Net
{
    internal static class ProtoParse
    {
        /// <summary>从帧 body 解析 Protobuf 消息；失败时 message 为 default。</summary>
        public static bool TryParse<T>(byte[] body, MessageParser<T> parser, out T message)
            where T : IMessage<T>
        {
            message = default;
            if (body == null || body.Length == 0)
            {
                return false;
            }

            try
            {
                message = parser.ParseFrom(body);
                return message != null;
            }
            catch
            {
                return false;
            }
        }
    }
}
