/// <summary>
/// 非阻塞 TCP + 可选 TLS（对标 sdk/net/TcpClient）。
/// 职责：connect/poll/send；MsgHeader 切帧后回调。
/// 线程：Unity 主线程 poll；连接在后台 Task 完成。
/// </summary>
using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System.Threading.Tasks;
using Rpg.Client.Config;
using Rpg.Client.Log;
using UnityEngine;

namespace Rpg.Client.Net
{
    public sealed class GameTcpClient : IDisposable
    {
        public delegate void MessageHandler(byte module, byte sub, byte[] body);
        public delegate void VoidHandler();

        private MessageHandler _onMessage;
        private VoidHandler _onConnected;
        private VoidHandler _onDisconnected;

        private TcpClient _tcp;
        private Stream _stream;
        private SslStream _ssl;
        private byte[] _recvBuf = new byte[8192];
        private int _recvLen;
        private readonly byte[] _readScratch = new byte[8192];
        private readonly Queue<byte[]> _sendQueue = new Queue<byte[]>();
        private readonly Queue<Action> _mainThreadQueue = new Queue<Action>();

        private volatile bool _connecting;
        private volatile bool _connected;
        private volatile bool _disposed;
        private ClientTlsConfig _tls;
        private string _host;

        public void SetOnMessage(MessageHandler cb) => _onMessage = cb;
        public void SetOnConnected(VoidHandler cb) => _onConnected = cb;
        public void SetOnDisconnected(VoidHandler cb) => _onDisconnected = cb;

        public bool IsConnected => _connected && !_connecting;
        public bool IsConnecting => _connecting;

        /// <summary>发起连接（异步）；结果通过 poll 回调。</summary>
        public bool Connect(string host, ushort port, ClientTlsConfig tls)
        {
            Disconnect();
            _tls = tls ?? new ClientTlsConfig();
            _host = host;
            _connecting = true;
            _connected = false;

            Task.Run(() => ConnectBackground(host, port));
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
            while (_mainThreadQueue.Count > 0)
            {
                _mainThreadQueue.Dequeue()?.Invoke();
            }

            if (!IsConnected || _stream == null)
            {
                return;
            }

            try
            {
                FlushSend();
                ReadAvailable();
                DecodeFrames();
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
                _ssl?.Dispose();
                _stream?.Dispose();
                _tcp?.Close();
            }
            catch
            {
                // ignore
            }

            _ssl = null;
            _stream = null;
            _tcp = null;
        }

        public void Dispose() => Disconnect();

        private void ConnectBackground(string host, ushort port)
        {
            try
            {
                var tcp = new TcpClient();
                tcp.NoDelay = true;
                tcp.Connect(host, port);
                Stream stream = tcp.GetStream();

                if (_tls.Enabled)
                {
                    var ssl = new SslStream(stream, false, ValidateCert);
                    ssl.AuthenticateAsClient(host);
                    stream = ssl;
                    EnqueueMain(() =>
                    {
                        _ssl = ssl;
                        FinishConnect(tcp, stream);
                    });
                }
                else
                {
                    EnqueueMain(() => FinishConnect(tcp, stream));
                }
            }
            catch (Exception ex)
            {
                EnqueueMain(() =>
                {
                    _connecting = false;
                    ClientLogger.Instance.ErrFormat("GameTcpClient：连接失败：{0}", ex.Message);
                    NotifyDisconnected();
                });
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

            return errors == SslPolicyErrors.None;
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
                EnsureRecvCapacity(_recvLen + read);
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
            }

            var expanded = new byte[newSize];
            Buffer.BlockCopy(_recvBuf, 0, expanded, 0, _recvLen);
            _recvBuf = expanded;
        }

        private void FlushSend()
        {
            while (true)
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
            }
        }

        private void DecodeFrames()
        {
            while (PacketCodec.TryDecode(_recvBuf, ref _recvLen, out var module, out var sub, out var body))
            {
                _onMessage?.Invoke(module, sub, body);
            }
        }

        private void NotifyDisconnected()
        {
            if (_disposed)
            {
                return;
            }

            Disconnect();
            _onDisconnected?.Invoke();
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
