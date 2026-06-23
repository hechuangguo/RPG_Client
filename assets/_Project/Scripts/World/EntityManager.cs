/// <summary>
/// 实体管理。职责：本地/远程玩家 spawn 与 despawn。
/// 内置轻量对象池，避免频繁 Instantiate/Destroy 造成的 GC 压力。
/// </summary>
using System.Collections.Generic;
using Rpg.Client.Log;
using Rpg.Proto.MapData;
using UnityEngine;

namespace Rpg.Client.World
{
    public sealed class EntityManager : MonoBehaviour
    {
        [SerializeField] private GameObject _playerPrefab;
        [SerializeField] private Transform _entityRoot;

        /// <summary>最大池容量，超出后直接 Destroy。</summary>
        private const int PoolCapacity = 32;

        private readonly Dictionary<ulong, GameObject> _entitiesById = new Dictionary<ulong, GameObject>();
        private readonly Queue<GameObject> _pool = new Queue<GameObject>();
        private GameObject _localPlayer;
        private ulong _localUserId;

        public Vector3 LocalPosition => _localPlayer != null ? _localPlayer.transform.position : Vector3.zero;

        private void Awake()
        {
            EnsureEntityRoot();
            EnsurePlayerPrefab();
        }

        public void Clear()
        {
            if (_entityRoot != null)
            {
                foreach (var pair in _entitiesById)
                {
                    ReturnToPool(pair.Value);
                }
            }

            _entitiesById.Clear();
            _localPlayer = null;
            _localUserId = 0;
        }

        public void SpawnLocalPlayer(ulong userId, string name, float x, float y, float z)
        {
            _localUserId = userId;
            if (_localPlayer != null)
            {
                _entitiesById.Remove(_localUserId);
                ReturnToPool(_localPlayer);
            }

            _localPlayer = RentFromPool(name, new Vector3(x, y, z));
            if (_localPlayer == null)
            {
                return;
            }

            _entitiesById[userId] = _localPlayer;
        }

        public void SpawnRemote(S2CSpawnEntity spawn)
        {
            if (spawn?.Pos == null)
            {
                return;
            }

            if (spawn.EntityId == _localUserId || _entitiesById.ContainsKey(spawn.EntityId))
            {
                return;
            }

            var go = RentFromPool(spawn.Name, new Vector3(spawn.Pos.X, spawn.Pos.Y, spawn.Pos.Z));
            if (go == null)
            {
                return;
            }

            _entitiesById[spawn.EntityId] = go;
        }

        public void ApplyMove(S2CMoveNotify move)
        {
            if (move?.Pos == null)
            {
                return;
            }

            var pos = new Vector3(move.Pos.X, move.Pos.Y, move.Pos.Z);
            if (TryGetEntity(move.UserId, out var go))
            {
                go.transform.position = pos;
            }
        }

        public void Despawn(ulong entityId)
        {
            if (!_entitiesById.TryGetValue(entityId, out var go))
            {
                return;
            }

            _entitiesById.Remove(entityId);
            if (go == _localPlayer)
            {
                _localPlayer = null;
                _localUserId = 0;
            }

            ReturnToPool(go);
        }

        public void SetLocalPosition(Vector3 pos)
        {
            if (_localPlayer != null)
            {
                _localPlayer.transform.position = pos;
            }
        }

        private bool TryGetEntity(ulong id, out GameObject go)
        {
            if (_localUserId != 0 && id == _localUserId && _localPlayer != null)
            {
                go = _localPlayer;
                return true;
            }

            return _entitiesById.TryGetValue(id, out go) && go != null;
        }

        /// <summary>从池中取出一个实体，或 Instantiate 新实体。</summary>
        private GameObject RentFromPool(string entityName, Vector3 pos)
        {
            EnsureEntityRoot();
            EnsurePlayerPrefab();
            if (_playerPrefab == null || _entityRoot == null)
            {
                ClientLogger.Instance.Err("EntityManager：缺少 PlayerPrefab 或 EntityRoot，无法创建实体");
                return null;
            }

            GameObject go;
            if (_pool.Count > 0)
            {
                go = _pool.Dequeue();
                go.SetActive(true);
            }
            else
            {
                go = Instantiate(_playerPrefab, _entityRoot);
            }

            go.name = string.IsNullOrEmpty(entityName) ? "Entity" : entityName;
            go.transform.position = pos;
            return go;
        }

        /// <summary>归还实体到池中（超容量则 Destroy）。</summary>
        private void ReturnToPool(GameObject go)
        {
            if (go == null)
            {
                return;
            }

            if (_pool.Count < PoolCapacity)
            {
                go.SetActive(false);
                go.name = "Entity_Pooled";
                _pool.Enqueue(go);
            }
            else
            {
                Destroy(go);
            }
        }

        private void EnsureEntityRoot()
        {
            if (_entityRoot != null)
            {
                return;
            }

            var rootGo = new GameObject("EntityRoot");
            rootGo.transform.SetParent(transform, false);
            _entityRoot = rootGo.transform;
            ClientLogger.Instance.Info("EntityManager：已自动创建 EntityRoot");
        }

        private void EnsurePlayerPrefab()
        {
            if (_playerPrefab != null)
            {
                return;
            }

            _playerPrefab = CreateRuntimePlayerPrefab();
            ClientLogger.Instance.Info("EntityManager：未配置 PlayerPrefab，已使用 Capsule 占位");
        }

        private GameObject CreateRuntimePlayerPrefab()
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            go.name = "PlayerPrefab_Runtime";
            go.SetActive(false);
            go.transform.SetParent(transform, false);

            var renderer = go.GetComponent<Renderer>();
            if (renderer != null)
            {
                renderer.material = new Material(Shader.Find("Standard"))
                {
                    color = new Color(0.25f, 0.55f, 1f)
                };
            }

            return go;
        }
    }
}
