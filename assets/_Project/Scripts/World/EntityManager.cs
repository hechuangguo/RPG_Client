/// <summary>
/// 实体管理（对标 game/EntityManager）。
/// 职责：Spawn/Move/Despawn 3D 实体。
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

        private readonly Dictionary<ulong, GameObject> _entitiesById = new Dictionary<ulong, GameObject>();
        private GameObject _localPlayer;
        private ulong _localUserId;

        public Vector3 LocalPosition => _localPlayer != null ? _localPlayer.transform.position : Vector3.zero;

        public void Clear()
        {
            if (_entityRoot != null)
            {
                foreach (Transform child in _entityRoot)
                {
                    Destroy(child.gameObject);
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
                Destroy(_localPlayer);
            }

            _localPlayer = CreateEntity(name, new Vector3(x, y, z));
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

            var go = CreateEntity(spawn.Name, new Vector3(spawn.Pos.X, spawn.Pos.Y, spawn.Pos.Z));
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

            Destroy(go);
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

        private GameObject CreateEntity(string name, Vector3 pos)
        {
            if (_playerPrefab == null || _entityRoot == null)
            {
                ClientLogger.Instance.Err("EntityManager：缺少 PlayerPrefab 或 EntityRoot，无法创建实体");
                return null;
            }

            var go = Instantiate(_playerPrefab, _entityRoot);
            go.name = string.IsNullOrEmpty(name) ? "Entity" : name;
            go.transform.position = pos;
            return go;
        }
    }
}
