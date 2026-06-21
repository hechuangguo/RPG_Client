/// <summary>
/// 客户端日志（文件 + Unity Console）。
/// </summary>
using System;
using System.IO;
using UnityEngine;

namespace Rpg.Client.Log
{
    public sealed class ClientLogger
    {
        public static ClientLogger Instance { get; } = new ClientLogger();

        public enum Level
        {
            Info,
            Warn,
            Err
        }

        public Level MinLevel { get; set; } = Level.Info;

        private readonly object _lock = new object();
        private string _logPath;

        /// <summary>初始化日志路径（Boot 场景启动时调用一次）。</summary>
        public void Initialize(string repoRoot = null)
        {
            var root = repoRoot;
            if (string.IsNullOrEmpty(root))
            {
                root = Directory.GetParent(Application.dataPath)?.FullName;
            }
            var logsDir = Path.Combine(root ?? ".", "logs");
            Directory.CreateDirectory(logsDir);
            _logPath = Path.Combine(logsDir, $"client_{DateTime.Now:yyyyMMdd}.log");
        }

        public void Info(string message) => Write(Level.Info, message);
        public void Warn(string message) => Write(Level.Warn, message);
        public void Err(string message) => Write(Level.Err, message);

        public void InfoFormat(string format, params object[] args) =>
            Write(Level.Info, string.Format(format, args));

        public void WarnFormat(string format, params object[] args) =>
            Write(Level.Warn, string.Format(format, args));

        public void ErrFormat(string format, params object[] args) =>
            Write(Level.Err, string.Format(format, args));

        private void Write(Level level, string message)
        {
            if (level < MinLevel)
            {
                return;
            }

            if (string.IsNullOrEmpty(_logPath))
            {
                Initialize();
            }

            var line = $"[{DateTime.Now:HH:mm:ss.fff}] [{LevelTag(level)}] {message}";
            lock (_lock)
            {
                File.AppendAllText(_logPath, line + Environment.NewLine);
            }

            if (level == Level.Err)
            {
                Debug.LogError(line);
            }
            else if (level == Level.Warn)
            {
                Debug.LogWarning(line);
            }
            else
            {
                Debug.Log(line);
            }
        }

        private static string LevelTag(Level level) =>
            level switch
            {
                Level.Warn => "WARN",
                Level.Err => "ERR",
                _ => "INFO"
            };
    }
}
