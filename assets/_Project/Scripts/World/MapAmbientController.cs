/// <summary>
/// 地图静态 NPC 生成与驱动。职责：读取 ambient.json → 程序化生成 NPC 实体 → 驱动闲逛/待机行为。
/// 使用程序化网格生成（无需 Prefab），后续可替换为美术预制体。
/// </summary>
using System.Collections.Generic;
using System.IO;
using Rpg.Client.Log;
using UnityEngine;

namespace Rpg.Client.World
{
    public sealed class MapAmbientController : MonoBehaviour
    {
        /// <summary>NPC 配置项。</summary>
        private struct NpcSpawnDef
        {
            public string Type;   // vendors / civilians / guards
            public int Count;
        }

        /// <summary>单个 NPC 运行时状态。</summary>
        private class NpcInstance
        {
            public GameObject GameObject;
            public Vector3 SpawnPos;
            public float WanderRadius;
            public float WanderTimer;
            public float WanderInterval;
            public Vector3 WanderTarget;
            public float Speed;
        }

        /// <summary>NPC 总数量上限。</summary>
        private const int MaxTotalNpcs = 30;
        /// <summary>NPC 默认出生半径。</summary>
        private const float DefaultSpawnRadius = 30f;
        /// <summary>闲逛移动速度。</summary>
        private const float DefaultWanderSpeed = 1.2f;
        /// <summary>NPC 身高（胶囊体）。</summary>
        private const float NpcHeight = 1.8f;
        /// <summary>NPC 半径。</summary>
        private const float NpcRadius = 0.4f;

        [SerializeField] private Transform _npcRoot;
        [SerializeField] private float _spawnRadius = DefaultSpawnRadius;

        private readonly List<NpcInstance> _npcs = new List<NpcInstance>();
        private readonly Dictionary<string, Color> _typeColors = new Dictionary<string, Color>
        {
            { "vendors", new Color(0.2f, 0.7f, 0.2f) },     // 绿色 — 商人
            { "civilians", new Color(0.6f, 0.5f, 0.3f) },   // 棕色 — 市民
            { "guards", new Color(0.7f, 0.2f, 0.2f) },      // 红色 — 守卫
        };

        /// <summary>已生成的 NPC 数量。</summary>
        public int NpcCount => _npcs.Count;

        private void Awake()
        {
            if (_npcRoot == null)
            {
                var go = new GameObject("NPCs");
                go.transform.SetParent(transform, false);
                _npcRoot = go.transform;
            }
        }

        /// <summary>加载地图时调用：读取 ambient.json 并生成所有 NPC。</summary>
        public void LoadAmbient(uint mapId)
        {
            Clear();

            var defs = ParseAmbientJson(mapId);
            if (defs == null || defs.Count == 0)
            {
                ClientLogger.Instance.WarnFormat("MapAmbient：地图 {0} 无 ambient 配置，跳过 NPC 生成", mapId);
                return;
            }

            var totalCount = 0;
            foreach (var def in defs)
            {
                totalCount += def.Count;
            }

            if (totalCount > MaxTotalNpcs)
            {
                ClientLogger.Instance.WarnFormat(
                    "MapAmbient：NPC 总数 {0} 超出上限 {1}，按比例缩减", totalCount, MaxTotalNpcs);
                var scale = (float)MaxTotalNpcs / totalCount;
                for (var i = 0; i < defs.Count; i++)
                {
                    var d = defs[i];
                    d.Count = Mathf.Max(1, Mathf.RoundToInt(d.Count * scale));
                    defs[i] = d;
                }

                totalCount = MaxTotalNpcs;
            }

            foreach (var def in defs)
            {
                if (!_typeColors.TryGetValue(def.Type, out var color))
                {
                    color = Color.gray;
                }

                for (var i = 0; i < def.Count; i++)
                {
                    SpawnNpc(def.Type, i, color);
                }
            }

            ClientLogger.Instance.InfoFormat("MapAmbient：地图 {0} 生成 {1} 个 NPC", mapId, _npcs.Count);
        }

        /// <summary>每帧驱动：闲逛移动。</summary>
        private void Update()
        {
            if (_npcs.Count == 0)
            {
                return;
            }

            var dt = Time.deltaTime;
            foreach (var npc in _npcs)
            {
                UpdateWander(npc, dt);
            }
        }

        /// <summary>清空所有 NPC。</summary>
        public void Clear()
        {
            foreach (var npc in _npcs)
            {
                if (npc.GameObject != null)
                {
                    Destroy(npc.GameObject);
                }
            }

            _npcs.Clear();
        }

        /// <summary>读取 Common/map/{mapId}/ambient.json，返回 NPC 配置列表。</summary>
        private static List<NpcSpawnDef> ParseAmbientJson(uint mapId)
        {
            // 优先使用 MapDataLoader 统一路径（Common/map/），回退到旧 map/ 目录
            var mapDir = MapDataLoader.GetMapDir(mapId);
            var candidates = new[]
            {
                Path.Combine(mapDir, "ambient.json"),
                Path.GetFullPath(Path.Combine(Application.dataPath, "..", "map", mapId.ToString(), "ambient.json")),
                Path.Combine(Application.streamingAssetsPath, "map", mapId.ToString(), "ambient.json")
            };

            string path = null;
            foreach (var candidate in candidates)
            {
                if (File.Exists(candidate))
                {
                    path = candidate;
                    break;
                }
            }

            if (path == null)
            {
                ClientLogger.Instance.WarnFormat(
                    "MapAmbient：未找到 ambient 配置 map/{0}/ambient.json（已检查工程 map 与 StreamingAssets）", mapId);
                return null;
            }

            try
            {
                var json = File.ReadAllText(path).Trim();
                var defs = new List<NpcSpawnDef>();

                // 简单手动解析，避免依赖第三方 JSON 库
                // 格式：{ "vendors": 5, "civilians": 10, "guards": 3 }
                var content = json.Trim('{', '}', ' ', '\r', '\n', '\t');
                var pairs = content.Split(',');
                foreach (var pair in pairs)
                {
                    var colonIdx = pair.IndexOf(':');
                    if (colonIdx < 0)
                    {
                        continue;
                    }

                    var key = pair.Substring(0, colonIdx).Trim().Trim('"');
                    var valStr = pair.Substring(colonIdx + 1).Trim();
                    if (int.TryParse(valStr, out var count) && count > 0)
                    {
                        defs.Add(new NpcSpawnDef { Type = key, Count = count });
                    }
                }

                return defs.Count > 0 ? defs : null;
            }
            catch (System.Exception ex)
            {
                ClientLogger.Instance.ErrFormat("MapAmbient：解析 ambient.json 失败 {0}", ex.Message);
                return null;
            }
        }

        /// <summary>程序化生成一个 NPC GameObject（胶囊体 + 名字标签）。</summary>
        private void SpawnNpc(string typeName, int index, Color color)
        {
            var go = new GameObject($"NPC_{typeName}_{index}");
            go.transform.SetParent(_npcRoot, false);

            // 随机出生位置
            var angle = Random.Range(0f, Mathf.PI * 2f);
            var dist = Random.Range(0f, _spawnRadius);
            var spawnPos = new Vector3(
                Mathf.Cos(angle) * dist,
                0,
                Mathf.Sin(angle) * dist);
            go.transform.position = spawnPos;

            // 身体 — 胶囊体
            var body = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            body.name = "Body";
            body.transform.SetParent(go.transform, false);
            body.transform.localPosition = new Vector3(0, NpcHeight * 0.5f, 0);
            body.transform.localScale = new Vector3(NpcRadius * 2f, NpcHeight * 0.5f, NpcRadius * 2f);
            var renderer = body.GetComponent<Renderer>();
            if (renderer != null)
            {
                renderer.material = new Material(Shader.Find("Standard"))
                {
                    color = color
                };
            }

            // 名字标签 — 3D Text (使用 TextMesh 或空 GameObject 占位)
            var labelGo = new GameObject("NameLabel");
            labelGo.transform.SetParent(go.transform, false);
            labelGo.transform.localPosition = new Vector3(0, NpcHeight + 0.3f, 0);
            var textMesh = labelGo.AddComponent<TextMesh>();
            textMesh.text = $"{typeName}#{index + 1}";
            textMesh.fontSize = 12;
            textMesh.color = Color.white;
            textMesh.alignment = TextAlignment.Center;
            textMesh.anchor = TextAnchor.MiddleCenter;
            textMesh.characterSize = 0.1f;

            // 注册到运行时列表
            var npc = new NpcInstance
            {
                GameObject = go,
                SpawnPos = spawnPos,
                WanderRadius = Random.Range(3f, 8f),
                WanderInterval = Random.Range(2f, 5f),
                WanderTimer = Random.Range(0f, 2f),
                Speed = DefaultWanderSpeed * Random.Range(0.7f, 1.3f),
            };
            npc.WanderTarget = GetRandomWanderTarget(npc);
            _npcs.Add(npc);
        }

        /// <summary>驱动单个 NPC 闲逛。</summary>
        private static void UpdateWander(NpcInstance npc, float dt)
        {
            if (npc.GameObject == null)
            {
                return;
            }

            npc.WanderTimer -= dt;
            if (npc.WanderTimer <= 0f)
            {
                npc.WanderTimer = npc.WanderInterval;
                npc.WanderTarget = GetRandomWanderTarget(npc);
            }

            var target = npc.WanderTarget;
            target.y = npc.GameObject.transform.position.y;
            var dir = target - npc.GameObject.transform.position;
            if (dir.sqrMagnitude < 0.01f)
            {
                return;
            }

            var move = dir.normalized * (npc.Speed * dt);
            if (move.sqrMagnitude > dir.sqrMagnitude)
            {
                move = dir;
            }

            npc.GameObject.transform.position += move;

            // 面向移动方向
            if (dir.sqrMagnitude > 0.001f)
            {
                var facing = new Vector3(dir.x, 0, dir.z).normalized;
                npc.GameObject.transform.forward = facing;
            }
        }

        private static Vector3 GetRandomWanderTarget(NpcInstance npc)
        {
            var angle = Random.Range(0f, Mathf.PI * 2f);
            var dist = Random.Range(0f, npc.WanderRadius);
            return npc.SpawnPos + new Vector3(
                Mathf.Cos(angle) * dist,
                0,
                Mathf.Sin(angle) * dist);
        }

        private void OnDestroy()
        {
            Clear();
        }
    }
}
