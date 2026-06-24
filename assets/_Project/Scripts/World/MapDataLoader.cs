/// <summary>
/// 统一地图数据加载器。从 Common/map/{mapId}/ 读取所有 JSON 配置，
/// 提供 MapData 结构供 WorldController、MapAmbientController 使用。
/// 路径解析优先级：Application.dataPath/../Common/map/ → StreamingAssets/map/
/// </summary>
using System;
using System.Collections.Generic;
using System.IO;
using Rpg.Client.Log;
using UnityEngine;

namespace Rpg.Client.World
{
    /// <summary>单张地图的完整数据。</summary>
    public sealed class MapData
    {
        public MapMeta Meta;
        public List<SpawnPointDef> Spawns;
        public List<NpcSpawnDef> AmbientNpcs;
        public List<BuildingDef> Buildings;
        public CollisionDef Collision;
        public GroundDef Ground;
    }

    [Serializable]
    public sealed class MapMeta
    {
        public int mapId;
        public string name = string.Empty;
        public int version;
        public string scenePath = string.Empty;
        public int maxPlayers;
        public string description = string.Empty;

        public int SizeWidth;
        public int SizeHeight;
        public int TileWidth;
        public int TileHeight;
        public float DefaultSpawnX;
        public float DefaultSpawnY;
        public float DefaultSpawnZ;
    }

    [Serializable]
    public struct SpawnPointDef
    {
        public string id;
        public float x;
        public float y;
        public float z;
        public int npcTemplateId;
    }

    [Serializable]
    public struct NpcSpawnDef
    {
        public string Type;
        public int Count;
    }

    [Serializable]
    public struct BuildingDef
    {
        public string id;
        public string name;
        public float x;
        public float z;
        public float w;
        public float h;
    }

    [Serializable]
    public sealed class BuildingList { public List<BuildingDef> buildings; }

    [Serializable]
    public sealed class CollisionDef
    {
        public int width;
        public int height;
        public List<string> solid;
    }

    [Serializable]
    public sealed class GroundDef
    {
        public int tileWidth;
        public int tileHeight;
        public int width;
        public int height;
        public string defaultTile = "bluestone";
        public float grassRatio;
    }

    [Serializable]
    public sealed class SpawnList { public List<SpawnPointDef> spawns; }

    /// <summary>包裹 ambient.json 手动解析结果的可序列化结构。</summary>
    [Serializable]
    public sealed class AmbientData { public List<NpcSpawnDef> entries; }

    public static class MapDataLoader
    {
        private const string CommonMapDir = "Common/map";

        /// <summary>加载指定地图的全部数据。</summary>
        public static MapData Load(uint mapId)
        {
            var data = new MapData
            {
                Meta = LoadMeta(mapId),
                Spawns = LoadSpawns(mapId),
                AmbientNpcs = LoadAmbient(mapId),
                Buildings = LoadBuildings(mapId),
                Collision = LoadCollision(mapId),
                Ground = LoadGround(mapId)
            };

            ClientLogger.Instance.InfoFormat("MapDataLoader：地图 {0}({1}) 加载完成 — 场景={2} 出生点={3} NPC组={4} 建筑={5}",
                mapId, data.Meta?.name, data.Meta?.scenePath,
                data.Spawns?.Count ?? 0, data.AmbientNpcs?.Count ?? 0, data.Buildings?.Count ?? 0);

            return data;
        }

        /// <summary>获取地图根目录的绝对路径。</summary>
        public static string GetMapDir(uint mapId)
        {
            var candidates = new[]
            {
                Path.GetFullPath(Path.Combine(Application.dataPath, "..", CommonMapDir, mapId.ToString())),
                Path.Combine(Application.streamingAssetsPath, "map", mapId.ToString())
            };

            foreach (var candidate in candidates)
            {
                if (Directory.Exists(candidate))
                {
                    return candidate;
                }
            }

            // 返回首选路径（即使不存在，也用于错误提示）
            return candidates[0];
        }

        /// <summary>读取单个 JSON 文件的文本内容。</summary>
        private static string ReadJsonFile(uint mapId, string fileName)
        {
            var mapDir = GetMapDir(mapId);

            var candidates = new[]
            {
                Path.Combine(mapDir, fileName),
                Path.Combine(Application.streamingAssetsPath, "map", mapId.ToString(), fileName)
            };

            foreach (var candidate in candidates)
            {
                if (File.Exists(candidate))
                {
                    return File.ReadAllText(candidate);
                }
            }

            return null;
        }

        private static MapMeta LoadMeta(uint mapId)
        {
            var json = ReadJsonFile(mapId, "meta.json");
            if (string.IsNullOrEmpty(json))
            {
                ClientLogger.Instance.WarnFormat("MapDataLoader：地图 {0} 无 meta.json，使用默认值", mapId);
                return new MapMeta
                {
                    mapId = (int)mapId,
                    name = $"Map_{mapId}",
                    scenePath = string.Empty
                };
            }

            try
            {
                // JsonUtility 不支持嵌套对象内的大小写字段，手动解析关键字段
                var meta = new MapMeta { mapId = (int)mapId };

                // scenePath
                meta.scenePath = ExtractStringField(json, "scenePath");
                meta.name = ExtractStringField(json, "name");
                meta.description = ExtractStringField(json, "description");

                if (string.IsNullOrEmpty(meta.name))
                {
                    meta.name = $"Map_{mapId}";
                }

                // defaultSpawn
                var spawnMatch = System.Text.RegularExpressions.Regex.Match(
                    json, @"""defaultSpawn""\s*:\s*\{[^}]*""x""\s*:\s*([0-9.\-]+)[^}]*""y""\s*:\s*([0-9.\-]+)[^}]*""z""\s*:\s*([0-9.\-]+)");
                if (spawnMatch.Success)
                {
                    float.TryParse(spawnMatch.Groups[1].Value, out meta.DefaultSpawnX);
                    float.TryParse(spawnMatch.Groups[2].Value, out meta.DefaultSpawnY);
                    float.TryParse(spawnMatch.Groups[3].Value, out meta.DefaultSpawnZ);
                }

                int.TryParse(ExtractStringField(json, "maxPlayers"), out meta.maxPlayers);

                return meta;
            }
            catch (Exception ex)
            {
                ClientLogger.Instance.ErrFormat("MapDataLoader：解析 meta.json 失败 {0}", ex.Message);
                return new MapMeta { mapId = (int)mapId, name = $"Map_{mapId}" };
            }
        }

        private static List<SpawnPointDef> LoadSpawns(uint mapId)
        {
            var json = ReadJsonFile(mapId, "spawns.json");
            if (string.IsNullOrEmpty(json))
            {
                return null;
            }

            try
            {
                var list = JsonUtility.FromJson<SpawnList>(json);
                return list?.spawns;
            }
            catch (Exception ex)
            {
                ClientLogger.Instance.ErrFormat("MapDataLoader：解析 spawns.json 失败 {0}", ex.Message);
                return null;
            }
        }

        private static List<NpcSpawnDef> LoadAmbient(uint mapId)
        {
            var json = ReadJsonFile(mapId, "ambient.json");
            if (string.IsNullOrEmpty(json))
            {
                return null;
            }

            try
            {
                var defs = new List<NpcSpawnDef>();
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
            catch (Exception ex)
            {
                ClientLogger.Instance.ErrFormat("MapDataLoader：解析 ambient.json 失败 {0}", ex.Message);
                return null;
            }
        }

        private static List<BuildingDef> LoadBuildings(uint mapId)
        {
            var json = ReadJsonFile(mapId, "buildings.json");
            if (string.IsNullOrEmpty(json))
            {
                return null;
            }

            try
            {
                var list = JsonUtility.FromJson<BuildingList>(json);
                return list?.buildings;
            }
            catch (Exception ex)
            {
                ClientLogger.Instance.ErrFormat("MapDataLoader：解析 buildings.json 失败 {0}", ex.Message);
                return null;
            }
        }

        private static CollisionDef LoadCollision(uint mapId)
        {
            var json = ReadJsonFile(mapId, "collision.json");
            if (string.IsNullOrEmpty(json))
            {
                return null;
            }

            try
            {
                return JsonUtility.FromJson<CollisionDef>(json);
            }
            catch (Exception ex)
            {
                ClientLogger.Instance.ErrFormat("MapDataLoader：解析 collision.json 失败 {0}", ex.Message);
                return null;
            }
        }

        private static GroundDef LoadGround(uint mapId)
        {
            var json = ReadJsonFile(mapId, "ground.json");
            if (string.IsNullOrEmpty(json))
            {
                return null;
            }

            try
            {
                return JsonUtility.FromJson<GroundDef>(json);
            }
            catch (Exception ex)
            {
                ClientLogger.Instance.ErrFormat("MapDataLoader：解析 ground.json 失败 {0}", ex.Message);
                return null;
            }
        }

        /// <summary>从 JSON 文本中提取字符串字段值。</summary>
        private static string ExtractStringField(string json, string fieldName)
        {
            var pattern = $"\"{fieldName}\"\\s*:\\s*\"([^\"]*)\"";
            var match = System.Text.RegularExpressions.Regex.Match(json, pattern);
            return match.Success ? match.Groups[1].Value : string.Empty;
        }
    }
}
