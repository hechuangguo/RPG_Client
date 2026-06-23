/// <summary>
/// 世界场景控制器。职责：绑定 GameSession、加载地图、驱动 EntityManager。
/// 输入使用 Unity InputSystem，支持键盘(WASD/方向键)和手柄左摇杆。
/// </summary>
using Rpg.Client.Net;
using Rpg.Proto.Login;
using Rpg.Proto.MapData;
using UnityEngine;
using UnityEngine.InputSystem;

namespace Rpg.Client.World
{
    public sealed class WorldController : MonoBehaviour
    {
        /// <summary>移动包发送最小间隔（毫秒）。默认 100ms ≈ 10 包/秒，兼顾流畅度与带宽。</summary>
        private const long MoveSendIntervalMs = 100;

        [SerializeField] private EntityManager _entities;
        [SerializeField] private GameSession _gameSession;
        [SerializeField] private MapAmbientController _mapAmbient;
        [SerializeField] private float _moveSpeed = 5f;

        private ulong _localUserId;
        private bool _active;
        private Vector3 _lastSentPos;
        private bool _hasSentPos;
        private long _lastMoveSendMs;

        // InputSystem
        private InputAction _moveAction;

        /// <summary>本地玩家当前世界坐标。</summary>
        public Vector3 EntityPosition => _entities != null ? _entities.LocalPosition : Vector3.zero;

        private void OnEnable()
        {
            _moveAction = new InputAction("Move", InputActionType.Value);
            // 键盘主键位
            _moveAction.AddCompositeBinding("2DVector")
                .With("Up", "<Keyboard>/w")
                .With("Down", "<Keyboard>/s")
                .With("Left", "<Keyboard>/a")
                .With("Right", "<Keyboard>/d");
            // 键盘方向键
            _moveAction.AddCompositeBinding("2DVector")
                .With("Up", "<Keyboard>/upArrow")
                .With("Down", "<Keyboard>/downArrow")
                .With("Left", "<Keyboard>/leftArrow")
                .With("Right", "<Keyboard>/rightArrow");
            // 手柄左摇杆
            _moveAction.AddBinding("<Gamepad>/leftStick");

            _moveAction.Enable();
        }

        private void OnDisable()
        {
            _moveAction?.Disable();
            _moveAction?.Dispose();
            _moveAction = null;
        }

        public void LoadMap(S2CEnterGame enter)
        {
            _localUserId = enter.UserId;
            _active = true;
            _hasSentPos = false;
            _lastMoveSendMs = 0;
            _entities?.Clear();

            // 自动解析 MapAmbient 组件
            if (_mapAmbient == null)
            {
                _mapAmbient = GetComponent<MapAmbientController>();
                if (_mapAmbient == null)
                {
                    _mapAmbient = gameObject.AddComponent<MapAmbientController>();
                }
            }

            _mapAmbient?.LoadAmbient(enter.MapId);

            if (enter.Pos != null)
            {
                _entities?.SpawnLocalPlayer(enter.UserId, enter.Name, enter.Pos.X, enter.Pos.Y, enter.Pos.Z);
            }
        }

        public void BindSession(GameSession session)
        {
            if (_gameSession != null)
            {
                _gameSession.OnSpawn = null;
                _gameSession.OnMove = null;
                _gameSession.OnDespawn = null;
            }

            _gameSession = session;
            if (session == null)
            {
                return;
            }

            session.OnSpawn = spawn => _entities?.SpawnRemote(spawn);
            session.OnMove = move => _entities?.ApplyMove(move);
            session.OnDespawn = despawn => _entities?.Despawn(despawn.EntityId);
        }

        public void Leave()
        {
            _active = false;
            _localUserId = 0;
            _hasSentPos = false;
            _lastMoveSendMs = 0;
            if (_gameSession != null)
            {
                _gameSession.OnSpawn = null;
                _gameSession.OnMove = null;
                _gameSession.OnDespawn = null;
            }

            _entities?.Clear();
            _mapAmbient?.Clear();
        }

        private void Update()
        {
            if (!_active || _gameSession == null)
            {
                return;
            }

            var moveInput = _moveAction.ReadValue<Vector2>();
            if (Mathf.Abs(moveInput.x) < 0.01f && Mathf.Abs(moveInput.y) < 0.01f)
            {
                return;
            }

            var pos = _entities?.LocalPosition ?? Vector3.zero;
            var delta = new Vector3(moveInput.x, 0, moveInput.y).normalized * (_moveSpeed * Time.deltaTime);
            if (delta.sqrMagnitude < 1e-8f)
            {
                return;
            }

            pos += delta;
            _entities?.SetLocalPosition(pos);
            if (_hasSentPos && (pos - _lastSentPos).sqrMagnitude < 1e-8f)
            {
                return;
            }

            // 频率节流：两次发包间隔不低于 MoveSendIntervalMs
            var nowMs = (long)(UnityEngine.Time.realtimeSinceStartupAsDouble * 1000.0);
            if (_hasSentPos && (nowMs - _lastMoveSendMs) < MoveSendIntervalMs)
            {
                return;
            }

            _lastSentPos = pos;
            _hasSentPos = true;
            _lastMoveSendMs = nowMs;
            _gameSession.SendMove(_localUserId, pos.x, pos.y, pos.z, 0f, MoveType.Walk);
        }
    }
}
