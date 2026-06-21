/// <summary>
/// 登录/创角表单客户端校验。
/// </summary>
using System.Collections.Generic;
using System.Globalization;
using Rpg.Client.Net;

namespace Rpg.Client.Util
{
    public static class ClientInputValidator
    {
        private const int AccountMinLen = 4;
        private const int AccountMaxLen = 32;
        private const int PasswordMinLen = 6;
        private const int PasswordMaxLen = 32;
        private const int CharacterNameMinLen = 2;
        private const int CharacterNameMaxLen = 12;

        public static bool TryValidateZoneSelected(uint zoneId, out string error)
        {
            if (zoneId == 0)
            {
                error = "请先选择区服";
                return false;
            }

            error = string.Empty;
            return true;
        }

        /// <summary>登录前校验区服在最新列表中且网关可用。</summary>
        public static bool TryValidateZoneForLogin(
            uint zoneId, byte gameType, IReadOnlyList<GameZoneEntry> zones, out string error)
        {
            if (!TryValidateZoneSelected(zoneId, out error))
            {
                return false;
            }

            if (zones == null || zones.Count == 0)
            {
                error = "请先刷新区服列表";
                return false;
            }

            GameZoneEntry matched = null;
            foreach (var zone in zones)
            {
                if (zone.ZoneId == zoneId)
                {
                    matched = zone;
                    break;
                }
            }

            if (matched == null)
            {
                error = "所选区服已失效，请重新选择";
                return false;
            }

            if (!matched.Enabled || matched.LoadStatus == ZoneLoadStatus.Maintenance)
            {
                error = "所选区服维护中，请更换区服";
                return false;
            }

            if (matched.GatewayCount == 0)
            {
                error = "所选区服暂无可用网关，请确认 Gateway 已启动";
                return false;
            }

            if (gameType != matched.GameType)
            {
                error = "区服参数已过期，请重新选择区服";
                return false;
            }

            error = string.Empty;
            return true;
        }

        public static bool TryValidateAccount(string account, out string error)
        {
            if (string.IsNullOrWhiteSpace(account))
            {
                error = "请输入账号";
                return false;
            }

            var trimmed = account.Trim();
            if (trimmed.Length < AccountMinLen || trimmed.Length > AccountMaxLen)
            {
                error = $"账号长度须为 {AccountMinLen}–{AccountMaxLen} 个字符";
                return false;
            }

            foreach (var c in trimmed)
            {
                if (c < 32 || c > 126)
                {
                    error = "账号仅支持可打印 ASCII 字符";
                    return false;
                }
            }

            error = string.Empty;
            return true;
        }

        public static bool TryValidatePassword(string password, out string error)
        {
            if (string.IsNullOrEmpty(password))
            {
                error = "请输入密码";
                return false;
            }

            if (password.Length < PasswordMinLen || password.Length > PasswordMaxLen)
            {
                error = $"密码长度须为 {PasswordMinLen}–{PasswordMaxLen} 个字符";
                return false;
            }

            error = string.Empty;
            return true;
        }

        public static bool TryValidatePasswordMatch(string password, string confirm, out string error)
        {
            if (!TryValidatePassword(password, out error))
            {
                return false;
            }

            if (password != confirm)
            {
                error = "两次输入的密码不一致";
                return false;
            }

            error = string.Empty;
            return true;
        }

        public static bool TryValidateCharacterName(string name, out string error)
        {
            if (string.IsNullOrWhiteSpace(name))
            {
                error = "请输入角色名";
                return false;
            }

            var trimmed = name.Trim();
            var length = new StringInfo(trimmed).LengthInTextElements;
            if (length < CharacterNameMinLen || length > CharacterNameMaxLen)
            {
                error = $"角色名须为 {CharacterNameMinLen}–{CharacterNameMaxLen} 个字符";
                return false;
            }

            error = string.Empty;
            return true;
        }
    }
}
