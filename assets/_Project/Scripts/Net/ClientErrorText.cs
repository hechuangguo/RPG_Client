/// <summary>
/// 错误码到中文文案（对标 sdk/net/ClientErrorText）。
/// </summary>
using Rpg.Proto.Login;
using Rpg.Proto.System;

namespace Rpg.Client.Net
{
    public static class ClientErrorText
    {
        public static string LoginResultText(int code, string serverMsg)
        {
            if (code == (int)LoginResultCode.LoginResultOk)
            {
                return string.Empty;
            }

            var fallback = code == (int)LoginResultCode.LoginBadCredentials
                ? "账号或密码错误"
                : "服务器错误";
            return "登录失败：" + PreferServerMsg(serverMsg, fallback);
        }

        public static string RegisterResultText(int code, string serverMsg)
        {
            if (code == (int)RegisterResultCode.RegisterOk)
            {
                return string.Empty;
            }

            var fallback = code == (int)RegisterResultCode.RegisterAccountExists
                ? "账号已存在"
                : "服务器错误";
            return "注册失败：" + PreferServerMsg(serverMsg, fallback);
        }

        public static string CreateCharacterText(int code, string serverMsg)
        {
            if (code == (int)CreateCharacterResultCode.CreateCharacterOk)
            {
                return string.Empty;
            }

            var fallback = code == (int)CreateCharacterResultCode.CreateCharacterDuplicateName
                ? "角色名不可用"
                : "系统错误";
            return "创建角色失败：" + PreferServerMsg(serverMsg, fallback);
        }

        public static string GatewayErrorText(S2CError err)
        {
            if (err == null)
            {
                return "网关错误";
            }

            return PreferServerMsg(err.Msg, GatewayValidateText((GatewayValidateCode)err.Code));
        }

        public static string GatewayValidateText(GatewayValidateCode code)
        {
            return code switch
            {
                GatewayValidateCode.GatewayValidateUnknownMsg => "未知消息",
                GatewayValidateCode.GatewayValidateBadLength => "消息长度错误",
                GatewayValidateCode.GatewayValidateBadState => "状态不允许",
                GatewayValidateCode.GatewayValidateBadPayload => "消息内容无效",
                GatewayValidateCode.GatewayValidateRateLimited => "请求过于频繁",
                _ => "网关校验失败"
            };
        }

        public static string LocalErrorText(ClientLocalError err, LoginTimeoutContext ctx = LoginTimeoutContext.Generic)
        {
            return err switch
            {
                ClientLocalError.ConnectTimeout => "连接超时",
                ClientLocalError.ResponseTimeout => ctx switch
                {
                    LoginTimeoutContext.ZoneList => "区列表响应超时",
                    LoginTimeoutContext.LogoutTimeout => "离世界响应超时",
                    LoginTimeoutContext.UserList => "角色列表响应超时",
                    _ => "服务器响应超时"
                },
                ClientLocalError.ZoneListTimeout => "区列表响应超时",
                ClientLocalError.LogoutTimeout => "离世界响应超时",
                ClientLocalError.Disconnected => "连接已断开",
                ClientLocalError.ParseError => "消息解析失败",
                ClientLocalError.GatewayError => "网关错误",
                _ => "网络错误"
            };
        }

        private static string PreferServerMsg(string serverMsg, string fallback)
        {
            return string.IsNullOrWhiteSpace(serverMsg) ? fallback : serverMsg.Trim();
        }
    }
}
