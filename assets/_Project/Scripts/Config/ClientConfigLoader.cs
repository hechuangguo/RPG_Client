/// <summary>
/// client_config.xml 加载器。
/// </summary>
using System;
using System.IO;
using System.Xml.Linq;
using Rpg.Client.Log;
using UnityEngine;

namespace Rpg.Client.Config
{
    public sealed class ClientConfigLoader
    {
        public string LastError { get; private set; } = string.Empty;
        public uint WindowWidth { get; private set; } = 1280;
        public uint WindowHeight { get; private set; } = 720;
        public string LogLevel { get; private set; } = "info";
        public bool LogToConsole { get; private set; }
        public string LoginHost { get; private set; } = "127.0.0.1";
        public ushort LoginPort { get; private set; } = 9010;
        /// <summary>非空时覆盖服务端下发的 Gateway IP（联调/端口转发）。</summary>
        public string GatewayHost { get; private set; } = string.Empty;
        /// <summary>为 true 且未配置 gatewayHost 时，Gateway 与 Login 共用 loginHost。</summary>
        public bool GatewayUseLoginHost { get; private set; }
        public long ConnectTimeoutMs { get; private set; } = ClientTimingDefaults.ConnectTimeoutMs;
        public long ResponseTimeoutMs { get; private set; } = ClientTimingDefaults.ResponseTimeoutMs;
        public long ZoneListResponseTimeoutMs { get; private set; } = ClientTimingDefaults.ZoneListResponseTimeoutMs;
        public long HeartbeatIntervalMs { get; private set; } = ClientTimingDefaults.HeartbeatIntervalMs;
        public long MoveSendIntervalMs { get; private set; } = ClientTimingDefaults.MoveSendIntervalMs;
        public float MoveMinDelta { get; private set; } = ClientTimingDefaults.MoveMinDelta;
        public int MaxFlushBytesPerPoll { get; private set; } = ClientTimingDefaults.MaxFlushBytesPerPoll;
        public long LogoutTimeoutMs { get; private set; } = ClientTimingDefaults.LogoutTimeoutMs;
        public ClientTlsConfig Tls { get; } = new ClientTlsConfig();
        /// <summary>地图 CDN 根 URL（如 http://127.0.0.1:8765/rpg/map/）。空则仅用本地 Common/map。</summary>
        public string MapCdnBaseUrl { get; private set; } = string.Empty;
        /// <summary>Addressables Remote Catalog 完整 URL；空则从 manifest.cdn 拼接。</summary>
        public string AddressablesRemoteUrl { get; private set; } = string.Empty;

        /// <summary>从 StreamingAssets 或仓库 config/ 加载。</summary>
        public bool Load()
        {
            LastError = string.Empty;
            foreach (var path in ResolveConfigPaths())
            {
                if (!File.Exists(path))
                {
                    continue;
                }

                try
                {
                    ParseXml(File.ReadAllText(path));
                    ClientLogger.Instance.InfoFormat("ClientConfigLoader：已加载 {0}", path);
                    ApplyLogLevel();
                    return true;
                }
                catch (Exception ex)
                {
                    LastError = ex.Message;
                    ClientLogger.Instance.WarnFormat("ClientConfigLoader：解析失败 {0}：{1}", path, ex.Message);
                }
            }

            ClientLogger.Instance.Warn("ClientConfigLoader：未找到 client_config.xml，使用默认值");
            ApplyLogLevel();
            return false;
        }

        private static string[] ResolveConfigPaths()
        {
            var root = Directory.GetParent(Application.dataPath)?.FullName ?? ".";
            return new[]
            {
                Path.Combine(Application.streamingAssetsPath, "config", "client_config.xml"),
                Path.Combine(root, "config", "client_config.xml"),
                Path.Combine(root, "config", "client_config.xml.example")
            };
        }

        private void ParseXml(string content)
        {
            var doc = XDocument.Parse(content);
            var root = doc.Root ?? throw new InvalidOperationException("缺少 ClientConfig 根节点");

            WindowWidth = ParseUInt(root, "windowWidth", WindowWidth);
            WindowHeight = ParseUInt(root, "windowHeight", WindowHeight);
            LogLevel = root.Element("logLevel")?.Value?.Trim() ?? LogLevel;
            LogToConsole = ParseBool(root.Element("logToConsole")?.Value, LogToConsole);
            LoginHost = root.Element("loginHost")?.Value?.Trim() ?? LoginHost;
            LoginPort = (ushort)ParseUInt(root, "loginPort", LoginPort);
            GatewayHost = root.Element("gatewayHost")?.Value?.Trim() ?? string.Empty;
            GatewayUseLoginHost = ParseBool(root.Element("gatewayUseLoginHost")?.Value, GatewayUseLoginHost);
            ConnectTimeoutMs = ParseLong(root, "connectTimeoutMs", ConnectTimeoutMs);
            ResponseTimeoutMs = ParseLong(root, "responseTimeoutMs", ResponseTimeoutMs);
            ZoneListResponseTimeoutMs = ParseLong(root, "zoneListResponseTimeoutMs", ZoneListResponseTimeoutMs);
            HeartbeatIntervalMs = ParseLong(root, "heartbeatIntervalMs", HeartbeatIntervalMs);
            MoveSendIntervalMs = ParseLong(root, "moveSendIntervalMs", MoveSendIntervalMs);
            MoveMinDelta = ParseFloat(root, "moveMinDelta", MoveMinDelta);
            MaxFlushBytesPerPoll = (int)ParseLong(root, "maxFlushBytesPerPoll", MaxFlushBytesPerPoll);
            LogoutTimeoutMs = ParseLong(root, "logoutTimeoutMs", LogoutTimeoutMs);
            MapCdnBaseUrl = root.Element("mapCdnBaseUrl")?.Value?.Trim() ?? string.Empty;
            AddressablesRemoteUrl = root.Element("addressablesRemoteUrl")?.Value?.Trim() ?? string.Empty;

            var tls = root.Element("Tls");
            if (tls != null)
            {
                Tls.Enabled = ParseBool(tls.Attribute("enabled")?.Value, Tls.Enabled);
                Tls.CaPath = tls.Attribute("ca")?.Value?.Trim() ?? Tls.CaPath;
                Tls.InsecureSkipVerify = ParseBool(tls.Attribute("insecureSkipVerify")?.Value, Tls.InsecureSkipVerify);
                Tls.MinVersion = tls.Attribute("minVersion")?.Value?.Trim() ?? Tls.MinVersion;
            }
        }

        private void ApplyLogLevel()
        {
            ClientLogger.Instance.MinLevel = LogLevel switch
            {
                "warn" => ClientLogger.Level.Warn,
                "err" => ClientLogger.Level.Err,
                _ => ClientLogger.Level.Info
            };
            ClientLogger.Instance.LogToConsole = LogToConsole;
        }

        private static uint ParseUInt(XElement root, string name, uint fallback)
        {
            var text = root.Element(name)?.Value?.Trim();
            return uint.TryParse(text, out var v) ? v : fallback;
        }

        private static long ParseLong(XElement root, string name, long fallback)
        {
            var text = root.Element(name)?.Value?.Trim();
            return long.TryParse(text, out var v) ? v : fallback;
        }

        private static float ParseFloat(XElement root, string name, float fallback)
        {
            var text = root.Element(name)?.Value?.Trim();
            return float.TryParse(text, out var v) ? v : fallback;
        }

        private static bool ParseBool(string text, bool fallback)
        {
            if (string.IsNullOrWhiteSpace(text))
            {
                return fallback;
            }

            return text == "1" || text.Equals("true", StringComparison.OrdinalIgnoreCase);
        }
    }
}
