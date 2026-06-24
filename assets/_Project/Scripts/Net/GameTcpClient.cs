/// <summary>
/// 非阻塞 TCP + 可选 TLS。职责：connect/poll/send；MsgHeader 切帧后回调。
/// 阻塞连接在后台线程执行，状态变更经主线程队列回传（IL2CPP 友好）。
/// </summary>
using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using Rpg.Client.Config;
using Rpg.Client.Log;
using UnityEngine;

namespace Rpg.Client.Net
{
    public sealed class GameTcpClient : IDisposable
    {
        public delegate void MessageHandler(byte module, byte sub, byte[] body);
        public delegate void VoidHandler();

        private const int RecvBufInitial = 8192;
        private static readonly int RecvBufMax = MsgHeader.Size + MsgHeader.MaxPacketSize;

        private MessageHandler _onMessage;
        private VoidHandler _onConnected;
        private VoidHandler _onDisconnected;

        private TcpClient _tcp;
        private Stream _stream;
        private SslStream _ssl;
        private byte[] _recvBuf = new byte[RecvBufInitial];
        private int _recvLen;
        private readonly byte[] _readScratch = new byte[RecvBufInitial];
        private readonly Queue<byte[]> _sendQueue = new Queue<byte[]>();
        private readonly Queue<Action> _mainThreadQueue = new Queue<Action>();
        private int _flushBytesThisPoll;

        private volatile bool _connecting;
        private volatile bool _connected;
        private volatile bool _disposed;
        private int _connectGeneration;
        private ClientTlsConfig _tls;
        private string _host;
        private X509Certificate2 _customCaCert;

        public void SetOnMessage(MessageHandler cb) => _onMessage = cb;
        public void SetOnConnected(VoidHandler cb) => _onConnected = cb;
        public void SetOnDisconnected(VoidHandler cb) => _onDisconnected = cb;

        public void ClearCallbacks()
        {
            _onMessage = null;
            _onConnected = null;
            _onDisconnected = null;
        }

        public bool IsConnected => _connected && !_connecting;
        public bool IsConnecting => _connecting;

        /// <summary>发起连接（异步）；结果通过 poll 回调。</summary>
        public bool Connect(string host, ushort port, ClientTlsConfig tls)
        {
            Disconnect();
            _tls = tls ?? new ClientTlsConfig();
            _host = host;
            LoadCustomCaIfNeeded();
            _connecting = true;
            _connected = false;
            var generation = ++_connectGeneration;

            var thread = new Thread(() => ConnectBackground(host, port, generation))
            {
                IsBackground = true,
                Name = "GameTcpClient.Connect"
            };
            thread.Start();
            return true;
        }

        public bool SendRaw(byte[] packet)
        {
            if (packet == null || packet.Length == 0 || !IsConnected)
            {
                return false;
            }

            lock (_sendQueue)
            {
                _sendQueue.Enqueue(packet);
            }

            return true;
        }

        public bool Send(byte module, byte sub, Google.Protobuf.IMessage body) =>
            SendRaw(PacketCodec.Encode(module, sub, body));

        /// <summary>主线程每帧调用。</summary>
        public void Poll()
        {
            // 加锁批量取出，避免后台线程 EnqueueMain 和主线程消费产生竞态
            Action[] pending;
            lock (_mainThreadQueue)
            {
                if (_mainThreadQueue.Count == 0)
                {
                    pending = null;
                }
                else
                {
                    pending = _mainThreadQueue.ToArray();
                    _mainThreadQueue.Clear();
                }
            }

            if (pending != null)
            {
                foreach (var action in pending)
                {
                    action?.Invoke();
                }
            }

            if (!IsConnected || _stream == null)
            {
                return;
            }

            try
            {
                _flushBytesThisPoll = 0;
                ReadAvailable();
                DecodeFrames();
                FlushSend();
                ShrinkRecvBufIfIdle();
            }
            catch (Exception ex)
            {
                ClientLogger.Instance.WarnFormat("GameTcpClient：IO 异常：{0}", ex.Message);
                NotifyDisconnected();
            }
        }

        public void Disconnect()
        {
            _connecting = false;
            _connected = false;
            _recvLen = 0;
            lock (_sendQueue)
            {
                _sendQueue.Clear();
            }

            try
            {
                if (_ssl != null)
                {
                    _ssl.Dispose();
                    _ssl = null;
                }
                else
                {
                    _stream?.Dispose();
                }
            }
            catch
            {
                // ignore
            }

            try
            {
                _tcp?.Close();
            }
            catch
            {
                // ignore
            }

            _stream = null;
            _tcp = null;
        }

        public void Dispose()
        {
            _disposed = true;
            Disconnect();
            _customCaCert?.Dispose();
            _customCaCert = null;
            ClearCallbacks();
        }

        private void ConnectBackground(string host, ushort port, int generation)
        {
            TcpClient tcp = null;
            SslStream ssl = null;
            Stream stream = null;

            try
            {
                tcp = new TcpClient();
                tcp.NoDelay = true;
                tcp.Connect(host, port);
                stream = tcp.GetStream();

                if (_tls.Enabled)
                {
                    ssl = new SslStream(stream, false, ValidateCert);
                    var protocols = ResolveSslProtocols(_tls.MinVersion);
                    ssl.AuthenticateAsClient(host, null, protocols, false);
                    stream = ssl;
                    EnqueueMain(() =>
                    {
                        if (generation != _connectGeneration)
                        {
                            DisposeConnectResources(tcp, ssl, stream);
                            return;
                        }

                        _ssl = ssl;
                        FinishConnect(tcp, stream);
                    });
                }
                else
                {
                    EnqueueMain(() =>
                    {
                        if (generation != _connectGeneration)
                        {
                            DisposeConnectResources(tcp, null, stream);
                            return;
                        }

                        FinishConnect(tcp, stream);
                    });
                }
            }
            catch (Exception ex)
            {
                DisposeConnectResources(tcp, ssl, stream);
                EnqueueMain(() =>
                {
                    if (generation != _connectGeneration)
                    {
                        return;
                    }

                    _connecting = false;
                    ClientLogger.Instance.ErrFormat("GameTcpClient：连接失败：{0}", ex.Message);
                    NotifyDisconnected();
                });
            }
        }

        private static void DisposeConnectResources(TcpClient tcp, SslStream ssl, Stream stream)
        {
            try
            {
                if (ssl != null)
                {
                    ssl.Dispose();
                }
                else
                {
                    stream?.Dispose();
                }
            }
            catch
            {
                // ignore
            }

            try
            {
                tcp?.Close();
            }
            catch
            {
                // ignore
            }
        }

        private void FinishConnect(TcpClient tcp, Stream stream)
        {
            _tcp = tcp;
            _stream = stream;
            _connecting = false;
            _connected = true;
            ClientLogger.Instance.InfoFormat("GameTcpClient：已连接 {0}", _host);
            _onConnected?.Invoke();
        }

        private bool ValidateCert(object sender, X509Certificate cert, X509Chain chain, SslPolicyErrors errors)
        {
            if (_tls.InsecureSkipVerify)
            {
                return true;
            }

            if (_customCaCert != null && cert != null)
            {
                using var serverCert = new X509Certificate2(cert);
                using var customChain = new X509Chain();
                customChain.ChainPolicy.ExtraStore.Add(_customCaCert);
                customChain.ChainPolicy.RevocationMode = X509RevocationMode.NoCheck;
                customChain.ChainPolicy.VerificationFlags =
                    X509VerificationFlags.AllowUnknownCertificateAuthority;
                return customChain.Build(serverCert);
            }

            return errors == SslPolicyErrors.None;
        }

        private void LoadCustomCaIfNeeded()
        {
            _customCaCert?.Dispose();
            _customCaCert = null;

            if (string.IsNullOrWhiteSpace(_tls?.CaPath) || _tls.InsecureSkipVerify)
            {
                return;
            }

            var relative = _tls.CaPath.Replace('/', Path.DirectorySeparatorChar);
            var repoRoot = Directory.GetParent(Application.dataPath)?.FullName ?? ".";
            foreach (var path in new[]
                     {
                         Path.Combine(Application.streamingAssetsPath, relative),
                         Path.Combine(repoRoot, relative)
                     })
            {
                if (!File.Exists(path))
                {
                    continue;
                }

                try
                {
                    _customCaCert = new X509Certificate2(path);
                    ClientLogger.Instance.InfoFormat("GameTcpClient：已加载 TLS CA {0}", path);
                    return;
                }
                catch (Exception ex)
                {
                    ClientLogger.Instance.WarnFormat("GameTcpClient：加载 TLS CA 失败 {0}：{1}", path, ex.Message);
                }
            }

            ClientLogger.Instance.WarnFormat("GameTcpClient：未找到 TLS CA 文件 {0}", _tls.CaPath);
        }

        private static SslProtocols ResolveSslProtocols(string minVersion)
        {
            if (string.IsNullOrWhiteSpace(minVersion))
            {
                return SslProtocols.Tls12;
            }

            switch (minVersion)
            {
                case "1.3":
                    ClientLogger.Instance.Warn("GameTcpClient：当前运行时未支持 TLS 1.3，已回退为 TLS 1.2");
                    return SslProtocols.Tls12;
                case "1.2":
                    return SslProtocols.Tls12;
                case "1.1":
                    return SslProtocols.Tls11;
                case "1.0":
                    return SslProtocols.Tls;
                default:
                    return SslProtocols.Tls12;
            }
        }

        private void ReadAvailable()
        {
            if (_tcp?.Available <= 0)
            {
                return;
            }

            int read;
            while (_tcp.Available > 0 && (read = _stream.Read(_readScratch, 0, _readScratch.Length)) > 0)
            {
                var required = _recvLen + read;
                if (required > RecvBufMax)
                {
                    ClientLogger.Instance.ErrFormat(
                        "GameTcpClient：接收缓冲超限 {0} > {1}，断开连接", required, RecvBufMax);
                    NotifyDisconnected();
                    return;
                }

                EnsureRecvCapacity(required);
                Buffer.BlockCopy(_readScratch, 0, _recvBuf, _recvLen, read);
                _recvLen += read;
            }
        }

        private void EnsureRecvCapacity(int required)
        {
            if (_recvBuf.Length >= required)
            {
                return;
            }

            var newSize = _recvBuf.Length;
            while (newSize < required)
            {
                newSize *= 2;
                if (newSize > RecvBufMax)
                {
                    newSize = RecvBufMax;
                    break;
                }
            }

            var expanded = new byte[newSize];
            Buffer.BlockCopy(_recvBuf, 0, expanded, 0, _recvLen);
            _recvBuf = expanded;
        }

        private void ShrinkRecvBufIfIdle()
        {
            if (_recvLen != 0 || _recvBuf.Length <= RecvBufInitial)
            {
                return;
            }

            _recvBuf = new byte[RecvBufInitial];
        }

        private void FlushSend()
        {
            var budget = ClientTimingDefaults.MaxFlushBytesPerPoll;

            while (_flushBytesThisPoll < budget)
            {
                byte[] packet;
                lock (_sendQueue)
                {
                    if (_sendQueue.Count == 0)
                    {
                        break;
                    }

                    packet = _sendQueue.Dequeue();
                }

                _stream.Write(packet, 0, packet.Length);
                _flushBytesThisPoll += packet.Length;
            }
        }

        private void DecodeFrames()
        {
            while (true)
            {
                if (!PacketCodec.TryDecode(
                        _recvBuf, ref _recvLen, out var module, out var sub, out var body, out var protocolError))
                {
                    if (protocolError)
                    {
                        ClientLogger.Instance.Err("GameTcpClient：协议帧非法，断开连接");
                        NotifyDisconnected();
                    }

                    break;
                }

                _onMessage?.Invoke(module, sub, body);
            }
        }

        private void NotifyDisconnected()
        {
            if (_disposed)
            {
                return;
            }

            var cb = _onDisconnected;
            Disconnect();
            cb?.Invoke();
        }

        private void EnqueueMain(Action action)
        {
            lock (_mainThreadQueue)
            {
                _mainThreadQueue.Enqueue(action);
            }
        }
    }
}
