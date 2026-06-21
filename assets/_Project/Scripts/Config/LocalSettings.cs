/// <summary>
/// 用户偏好持久化（对标 util/LocalSettings）。
/// Windows：%APPDATA%/RPGClient/settings.json
/// </summary>
using System;
using System.IO;
using UnityEngine;

namespace Rpg.Client.Config
{
    [Serializable]
    public sealed class LocalSettingsData
    {
        public string lastAccount = string.Empty;
        public uint lastZoneId;
        public byte lastGameType;
        public string lastZoneName = string.Empty;
        public bool rememberAccount;
    }

    /// <summary>本地 settings.json 读写。</summary>
    public sealed class LocalSettings
    {
        private LocalSettingsData _data = new LocalSettingsData();

        public string LastAccount => _data.lastAccount;
        public uint LastZoneId => _data.lastZoneId;
        public byte LastGameType => _data.lastGameType;
        public string LastZoneName => _data.lastZoneName;
        public bool RememberAccount => _data.rememberAccount;

        public string SettingsPath =>
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                "RPGClient", "settings.json");

        public bool Load()
        {
            var path = SettingsPath;
            if (!File.Exists(path))
            {
                return true;
            }

            try
            {
                var json = File.ReadAllText(path);
                _data = JsonUtility.FromJson<LocalSettingsData>(json) ?? new LocalSettingsData();
                return true;
            }
            catch
            {
                _data = new LocalSettingsData();
                return false;
            }
        }

        public bool Save()
        {
            try
            {
                var dir = Path.GetDirectoryName(SettingsPath);
                if (!string.IsNullOrEmpty(dir))
                {
                    Directory.CreateDirectory(dir);
                }

                File.WriteAllText(SettingsPath, JsonUtility.ToJson(_data, true));
                return true;
            }
            catch
            {
                return false;
            }
        }

        public void SetLastAccount(string account) => _data.lastAccount = account ?? string.Empty;
        public void SetLastZoneId(uint zoneId) => _data.lastZoneId = zoneId;
        public void SetLastGameType(byte gameType) => _data.lastGameType = gameType;
        public void SetLastZoneName(string name) => _data.lastZoneName = name ?? string.Empty;
        public void SetRememberAccount(bool remember) => _data.rememberAccount = remember;
    }
}
