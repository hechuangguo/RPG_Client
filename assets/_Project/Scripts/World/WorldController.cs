/// <summary>
/// 世界场景控制器。职责：绑定 GameSession、加载地图、驱动 EntityManager。
/// 进世界时通过 MapLoader 拉 CDN manifest + Addressables，或回退 Common/map meta.json。
/// </summary>
using System.Collections;
using Rpg.Client.Config;
using Rpg.Client.Log;
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
        private MapLoadResult _mapLoadResult;
        private MapData _mapData;
        private Camera _bootCamera;
        private ClientConfigLoader _config;

        private InputAction _moveAction;

        public event System.Action OnMapLoaded;
        public event System.Action<string> OnMapLoadError;

        public Vector3 EntityPosition => _entities != null ? _entities.LocalPosition : Vector3.zero;

        private void OnEnable()
        {
            _moveAction = new InputAction("Move", InputActionType.Value);
            _moveAction.AddCompositeBinding("2DVector")
                .With("Up", "<Keyboard>/w")
                .With("Down", "<Keyboard>/s")
                .With("Left", "<Keyboard>/a")
                .With("Right", "<Keyboard>/d");
            _moveAction.AddCompositeBinding("2DVector")
                .With("Up", "<Keyboard>/upArrow")
                .With("Down", "<Keyboard>/downArrow")
                .With("Left", "<Keyboard>/leftArrow")
                .With("Right", "<Keyboard>/rightArrow");
            _moveAction.AddBinding("<Gamepad>/leftStick");
            _moveAction.Enable();
        }

        private void OnDisable()
        {
            _moveAction?.Disable();
            _moveAction?.Dispose();
            _moveAction = null;
        }

        public void LoadMap(S2CEnterGame enter, uint modelId, ClientConfigLoader config)
        {
            _config = config;
            _localUserId = enter.UserId;
            _active = true;
            _hasSentPos = false;
            _lastMoveSendMs = 0;
            _entities?.Clear();

            EnsureMapAmbient();
            _mapAmbient?.LoadAmbient(enter.MapId);

            if (enter.Pos != null)
            {
                var mid = modelId > 0 ? modelId : CharacterDef.ModelDefault;
                _entities?.SpawnLocalPlayer(enter.UserId, enter.Name, mid, enter.Pos.X, enter.Pos.Y, enter.Pos.Z);
            }

            _bootCamera = Camera.main;
            StartCoroutine(LoadMapRoutine(enter.MapId));
        }

        private void EnsureMapAmbient()
        {
            if (_mapAmbient != null)
            {
                return;
            }

            _mapAmbient = GetComponent<MapAmbientController>();
            if (_mapAmbient == null)
            {
                _mapAmbient = gameObject.AddComponent<MapAmbientController>();
            }
        }

        private IEnumerator LoadMapRoutine(uint mapId)
        {
            MapLoadResult result = null;
            string error = null;

            yield return MapLoader.LoadAsync(
                mapId,
                _config,
                r => result = r,
                e => error = e);

            if (!string.IsNullOrEmpty(error))
            {
                ClientLogger.Instance.ErrFormat("WorldController：地图加载失败 — {0}", error);
                OnMapLoadError?.Invoke(error);
                yield break;
            }

            if (result == null)
            {
                OnMapLoadError?.Invoke("地图加载无结果");
                yield break;
            }

            _mapData = result.LocalData;
            _mapLoadResult = result;
            _mapSceneLoaded = true;

            FinalizeLoadedScene(result);
            OnMapLoaded?.Invoke();
        }

        private void FinalizeLoadedScene(MapLoadResult result)
        {
            Scene scene;
            if (result.UsedAddressables && result.SceneHandle.IsValid())
            {
                scene = result.SceneHandle.Result.Scene;
            }
            else
            {
                scene = SceneManager.GetSceneByPath(result.ScenePath);
            }

            if (!scene.IsValid())
            {
                for (var i = 0; i < SceneManager.sceneCount; i++)
                {
                    var candidate = SceneManager.GetSceneAt(i);
                    if (candidate.name.StartsWith("World_"))
                    {
                        scene = candidate;
                        break;
                    }
                }
            }

            if (!scene.IsValid())
            {
                ClientLogger.Instance.Warn("WorldController：未找到地图场景根对象，跳过挂载");
                return;
            }

            foreach (var rootObj in scene.GetRootGameObjects())
            {
                var cam = rootObj.GetComponent<Camera>();
                if (cam != null)
                {
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

                rootObj.transform.SetParent(transform, false);
            }

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

            if (_mapSceneLoaded && _mapLoadResult != null)
            {
                MapLoader.Unload(_mapLoadResult);
                _mapSceneLoaded = false;
                _mapLoadResult = null;
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

            var nowMs = (long)(Time.realtimeSinceStartupAsDouble * 1000.0);
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
