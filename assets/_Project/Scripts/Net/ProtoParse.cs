/// <summary>
/// Protobuf body 解析 helper；供 ClientMsgHandler 语义化 TryParse 包装复用。
/// </summary>
using Google.Protobuf;
using Rpg.Client.Log;

namespace Rpg.Client.Net
{
    internal static class ProtoParse
    {
        /// <summary>从帧 body 解析 Protobuf 消息；失败时 message 为 default。</summary>
        public static bool TryParse<T>(byte[] body, MessageParser<T> parser, out T message, string context = null)
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
            catch (InvalidProtocolBufferException ex)
            {
                var label = string.IsNullOrEmpty(context) ? "未知消息" : context;
                ClientLogger.Instance.WarnFormat("ProtoParse：解析失败 {0}：{1}", label, ex.Message);
                return false;
            }
        }
    }
}
