/// <summary>
/// 客户端日志（缓冲文件 + Unity Console）。
/// </summary>
using System;
using System.IO;
using UnityEngine;

namespace Rpg.Client.Log
{
    public sealed class ClientLogger : IDisposable
    {
        public static ClientLogger Instance { get; } = new ClientLogger();

        public enum Level
        {
            Info,
            Warn,
            Err
        }

        private const long FlushIntervalMs = 1000;

        public Level MinLevel { get; set; } = Level.Info;

        private readonly object _lock = new object();
        private string _logPath;
        private StreamWriter _writer;
        private long _lastFlushMs;

        /// <summary>初始化日志路径（幂等：已初始化则跳过，避免 GameBootstrap 与 GameApp 双重调用丢日志）。</summary>
        public void Initialize(string repoRoot = null)
        {
            lock (_lock)
            {
                if (_writer != null)
                {
                    return; // 已初始化，幂等跳过
                }
            }

            var root = repoRoot;
            if (string.IsNullOrEmpty(root))
            {
                root = Directory.GetParent(Application.dataPath)?.FullName;
            }

            var logsDir = Path.Combine(root ?? ".", "logs");
            Directory.CreateDirectory(logsDir);
            _logPath = Path.Combine(logsDir, $"client_{DateTime.Now:yyyyMMdd}.log");
            OpenWriter();
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

        public void Shutdown() => Dispose();

        public void Dispose()
        {
            lock (_lock)
            {
                try
                {
                    _writer?.Flush();
                    _writer?.Dispose();
                }
                catch
                {
                    // ignore
                }

                _writer = null;
            }
        }

        private void OpenWriter()
        {
            lock (_lock)
            {
                _writer?.Dispose();
                var stream = new FileStream(
                    _logPath,
                    FileMode.Append,
                    FileAccess.Write,
                    FileShare.Read);
                _writer = new StreamWriter(stream) { AutoFlush = false };
                _lastFlushMs = (long)(Time.realtimeSinceStartupAsDouble * 1000.0);
            }
        }

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
                EnsureWriter();
                _writer.WriteLine(line);
                var nowMs = (long)(Time.realtimeSinceStartupAsDouble * 1000.0);
                if (level == Level.Err || nowMs - _lastFlushMs >= FlushIntervalMs)
                {
                    _writer.Flush();
                    _lastFlushMs = nowMs;
                }
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

        private void EnsureWriter()
        {
            if (_writer == null)
            {
                OpenWriter();
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
