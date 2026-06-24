/// <summary>
/// 世界场景控制器。职责：绑定 GameSession、加载地图、驱动 EntityManager。
/// 进世界时通过 MapDataLoader 读取 Common/map/{mapId}/meta.json 获取场景路径，
/// 附加加载对应 Unity 3D 场景，离世界时卸载。
/// 输入使用 Unity InputSystem，支持键盘(WASD/方向键)和手柄左摇杆。
/// </summary>
using System.Collections;
using Rpg.Client.Net;
using Rpg.Proto.Login;
using Rpg.Proto.MapData;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.Rendering;
using UnityEngine.SceneManagement;

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
        private bool _mapSceneLoaded;
        private string _loadedScenePath;
        private MapData _mapData;
        private Camera _bootCamera;

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

            // 加载地图数据（从 Common/map/{mapId}/）
            _mapData = MapDataLoader.Load(enter.MapId);

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

            // 根据 meta.json 的 scenePath 附加加载 Unity 3D 场景
            if (!_mapSceneLoaded && _mapData?.Meta != null && !string.IsNullOrEmpty(_mapData.Meta.scenePath))
            {
                _bootCamera = Camera.main;
                _loadedScenePath = _mapData.Meta.scenePath;
                StartCoroutine(LoadMapScene());
            }
        }

        /// <summary>
        /// 异步附加加载地图场景，将 CityRoot 挂入 World 层级，
        /// 清理重复摄像机，并设置环境渲染设置。
        /// </summary>
        private IEnumerator LoadMapScene()
        {
            var op = SceneManager.LoadSceneAsync(_loadedScenePath, LoadSceneMode.Additive);
            while (op != null && !op.isDone)
            {
                yield return null;
            }

            _mapSceneLoaded = true;

            // 将场景根对象挂入 World 层级
            var scene = SceneManager.GetSceneByPath(_loadedScenePath);
            if (scene.IsValid())
            {
                foreach (var rootObj in scene.GetRootGameObjects())
                {
                    // 跳过摄像机（用 Boot 场景的）
                    var cam = rootObj.GetComponent<Camera>();
                    if (cam != null)
                    {
                        // 同步摄像机参数到 Boot 摄像机
                        if (_bootCamera != null)
                        {
                            _bootCamera.transform.position = cam.transform.position;
                            _bootCamera.transform.rotation = cam.transform.rotation;
                            _bootCamera.fieldOfView = cam.fieldOfView;
                            _bootCamera.farClipPlane = cam.farClipPlane;
                            _bootCamera.nearClipPlane = cam.nearClipPlane;
                            _bootCamera.clearFlags = cam.clearFlags;
                        }
                        Destroy(cam.gameObject);
                        continue;
                    }

                    // 将 CityRoot / 其他根对象挂入 World
                    rootObj.transform.SetParent(transform, false);
                }
            }

            // 附加加载不传递 RenderSettings，手动设置氛围
            RenderSettings.fog = true;
            RenderSettings.fogMode = FogMode.Linear;
            RenderSettings.fogColor = new Color(0.6f, 0.7f, 0.75f, 1f);
            RenderSettings.fogStartDistance = 40;
            RenderSettings.fogEndDistance = 120;
            RenderSettings.ambientMode = AmbientMode.Flat;
            RenderSettings.ambientLight = new Color(0.5f, 0.55f, 0.6f, 1f);
            RenderSettings.ambientIntensity = 1f;
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

            // 卸载地图场景
            if (_mapSceneLoaded && !string.IsNullOrEmpty(_loadedScenePath))
            {
                SceneManager.UnloadSceneAsync(_loadedScenePath);
                _mapSceneLoaded = false;
                _loadedScenePath = null;
            }

            _mapData = null;
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
