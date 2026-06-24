/// <summary>
/// 实体管理。职责：本地/远程玩家 spawn 与 despawn。
/// 按 model_id 选择对应的角色模型 Prefab（男大/男小/女大/女小）。
/// 内置轻量对象池，避免频繁 Instantiate/Destroy 造成的 GC 压力。
/// </summary>
using System.Collections.Generic;
using Rpg.Client.Log;
using Rpg.Client.Net;
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
        private readonly Dictionary<uint, GameObject> _modelPrefabs = new Dictionary<uint, GameObject>();
        private GameObject _localPlayer;
        private ulong _localUserId;

        public Vector3 LocalPosition => _localPlayer != null ? _localPlayer.transform.position : Vector3.zero;

        private void Awake()
        {
            EnsureEntityRoot();
            EnsureModelPrefabs();
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

        public void SpawnLocalPlayer(ulong userId, string name, uint modelId, float x, float y, float z)
        {
            _localUserId = userId;
            if (_localPlayer != null)
            {
                _entitiesById.Remove(_localUserId);
                ReturnToPool(_localPlayer);
            }

            _localPlayer = RentFromPool(name, modelId, new Vector3(x, y, z));
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

            var go = RentFromPool(spawn.Name, spawn.ModelId, new Vector3(spawn.Pos.X, spawn.Pos.Y, spawn.Pos.Z));
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
        private GameObject RentFromPool(string entityName, uint modelId, Vector3 pos)
        {
            EnsureEntityRoot();
            EnsureModelPrefabs();
            var prefab = GetModelPrefab(modelId);
            if (prefab == null || _entityRoot == null)
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
                go = Instantiate(prefab, _entityRoot);
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

        /// <summary>根据 model_id 获取对应的 Prefab，未找到时回退到默认。</summary>
        private GameObject GetModelPrefab(uint modelId)
        {
            if (_modelPrefabs.TryGetValue(modelId, out var prefab))
            {
                return prefab;
            }

            // 回退到 SerializeField 配置的 _playerPrefab 或第一个可用模型
            if (_playerPrefab != null)
            {
                return _playerPrefab;
            }

            foreach (var pair in _modelPrefabs)
            {
                return pair.Value;
            }

            return null;
        }

        /// <summary>初始化4种角色模型 Prefab（如未在 Inspector 配置）。</summary>
        private void EnsureModelPrefabs()
        {
            if (_modelPrefabs.Count > 0)
            {
                return;
            }

            // 如果 Inspector 配置了 _playerPrefab，用作默认模型(model_id=1)
            if (_playerPrefab == null)
            {
                _playerPrefab = CreateModelPrefab(CharacterDef.ModelMaleAdult);
                ClientLogger.Instance.Info("EntityManager：未配置 PlayerPrefab，已使用程序化模型占位");
            }

            _modelPrefabs[CharacterDef.ModelMaleAdult] = _playerPrefab;
            _modelPrefabs[CharacterDef.ModelMaleChild] = CreateModelPrefab(CharacterDef.ModelMaleChild);
            _modelPrefabs[CharacterDef.ModelFemaleAdult] = CreateModelPrefab(CharacterDef.ModelFemaleAdult);
            _modelPrefabs[CharacterDef.ModelFemaleChild] = CreateModelPrefab(CharacterDef.ModelFemaleChild);
        }

        /// <summary>
        /// 程序化生成角色模型 Prefab。由基础几何体组合，仙侠风格配色。
        /// model_id: 1=男大 2=男小 3=女大 4=女小
        /// </summary>
        private GameObject CreateModelPrefab(uint modelId)
        {
            var go = new GameObject($"PlayerModel_{modelId}");
            go.SetActive(false);
            go.transform.SetParent(transform, false);

            var isMale = modelId == CharacterDef.ModelMaleAdult || modelId == CharacterDef.ModelMaleChild;
            var isAdult = modelId == CharacterDef.ModelMaleAdult || modelId == CharacterDef.ModelFemaleAdult;

            // 身体尺寸
            var bodyHeight = isAdult ? 1.2f : 0.9f;
            var bodyRadius = isAdult ? 0.35f : 0.28f;
            var headRadius = isAdult ? 0.25f : 0.2f;
            var totalHeight = bodyHeight + headRadius * 2f;

            // 仙侠风格配色
            Color robeColor, headColor, hairColor;
            if (isMale)
            {
                robeColor = isAdult ? new Color(0.15f, 0.35f, 0.55f, 1f) : new Color(0.2f, 0.45f, 0.65f, 1f);
                headColor = new Color(0.95f, 0.82f, 0.68f, 1f);
                hairColor = isAdult ? new Color(0.1f, 0.08f, 0.06f, 1f) : new Color(0.15f, 0.1f, 0.08f, 1f);
            }
            else
            {
                robeColor = isAdult ? new Color(0.55f, 0.2f, 0.45f, 1f) : new Color(0.65f, 0.3f, 0.5f, 1f);
                headColor = new Color(0.95f, 0.85f, 0.72f, 1f);
                hairColor = isAdult ? new Color(0.2f, 0.1f, 0.05f, 1f) : new Color(0.3f, 0.15f, 0.1f, 1f);
            }

            // 身体 — Capsule
            var body = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            body.name = "Body";
            body.transform.SetParent(go.transform, false);
            body.transform.localPosition = new Vector3(0, bodyHeight * 0.5f, 0);
            body.transform.localScale = new Vector3(bodyRadius * 2f, bodyHeight * 0.5f, bodyRadius * 2f);
            SetMaterial(body, robeColor, 0.5f);

            // 头部 — Sphere
            var head = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            head.name = "Head";
            head.transform.SetParent(go.transform, false);
            head.transform.localPosition = new Vector3(0, bodyHeight + headRadius, 0);
            head.transform.localScale = Vector3.one * (headRadius * 2f);
            SetMaterial(head, headColor, 0.6f);

            // 发髻 — Sphere (仙侠发型)
            var hair = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            hair.name = "Hair";
            hair.transform.SetParent(go.transform, false);
            hair.transform.localPosition = new Vector3(0, bodyHeight + headRadius * 1.5f, -headRadius * 0.3f);
            hair.transform.localScale = Vector3.one * (headRadius * 1.2f);
            SetMaterial(hair, hairColor, 0.4f);

            // 腰带 — Cube (装饰)
            var belt = GameObject.CreatePrimitive(PrimitiveType.Cube);
            belt.name = "Belt";
            belt.transform.SetParent(go.transform, false);
            belt.transform.localPosition = new Vector3(0, bodyHeight * 0.35f, 0);
            belt.transform.localScale = new Vector3(bodyRadius * 2.2f, 0.1f, bodyRadius * 2.2f);
            SetMaterial(belt, isMale ? new Color(0.2f, 0.15f, 0.08f, 1f) : new Color(0.8f, 0.65f, 0.2f, 1f), 0.3f);

            // 名字标签
            var labelGo = new GameObject("NameLabel");
            labelGo.transform.SetParent(go.transform, false);
            labelGo.transform.localPosition = new Vector3(0, totalHeight + 0.3f, 0);
            var textMesh = labelGo.AddComponent<TextMesh>();
            textMesh.fontSize = 14;
            textMesh.color = Color.white;
            textMesh.alignment = TextAlignment.Center;
            textMesh.anchor = TextAnchor.MiddleCenter;
            textMesh.characterSize = 0.1f;

            return go;
        }

        private static void SetMaterial(GameObject go, Color color, float smoothness)
        {
            var renderer = go.GetComponent<Renderer>();
            if (renderer == null)
            {
                return;
            }

            var mat = new Material(Shader.Find("Standard"))
            {
                color = color
            };
            mat.SetFloat("_Glossiness", smoothness);
            renderer.material = mat;
        }
    }
}
