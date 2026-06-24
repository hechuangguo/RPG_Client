/// <summary>
/// MapExport Editor 工具：导出 client + server 双端包、navmesh.bin。
/// 菜单：RPG → Export Map Package
/// </summary>
using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using Rpg.Client.World;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.AI;
using UnityEngine.SceneManagement;

namespace Rpg.Client.EditorTools
{
    public static class MapExportWindow
    {
        private const int ManifestVersion = 1;
        private const string CoordSystem = "unity_y_up";
        private const string NavMeshMagic = "RPGN";
        private const string ScenePath = "assets/_Project/Scenes/World_1002.unity";

        [MenuItem("RPG/Export Map Package")]
        public static void ExportMapPackage()
        {
            var scene = SceneManager.GetActiveScene();
            if (!scene.IsValid())
            {
                EditorUtility.DisplayDialog("MapExport", "无有效场景", "确定");
                return;
            }

            var mapId = ParseMapId(scene.name);
            var outRoot = EditorUtility.OpenFolderPanel("选择 CDN 根目录（rpg/map）", "build/map_cdn/rpg/map", "");
            if (string.IsNullOrEmpty(outRoot))
            {
                return;
            }

            ExportDualPackage(mapId, scene.name, outRoot);
            EditorUtility.DisplayDialog("MapExport", $"已导出双端包至\n{outRoot}", "确定");
        }

        /// <summary>batchmode：-executeMethod Rpg.Client.EditorTools.MapExportWindow.ExportMapPackageBatch</summary>
        public static void ExportMapPackageBatch()
        {
            var scenePath = ScenePath;
            if (File.Exists(scenePath))
            {
                EditorSceneManager.OpenScene(scenePath, OpenSceneMode.Single);
            }

            var root = Environment.GetEnvironmentVariable("RPG_MAP_CDN_ROOT");
            if (string.IsNullOrEmpty(root))
            {
                root = Path.GetFullPath(Path.Combine(Application.dataPath, "..", "build", "map_cdn", "rpg", "map"));
            }

            ExportDualPackage(1002, "World_1002", root);
            EditorApplication.Exit(0);
        }

        public static void ExportDualPackage(int mapId, string sceneName, string outRoot)
        {
            Directory.CreateDirectory(outRoot);

            var clientRoot = Path.Combine(outRoot, "client", mapId.ToString());
            var serverRoot = Path.Combine(outRoot, "server", mapId.ToString());
            Directory.CreateDirectory(Path.Combine(clientRoot, "logic"));
            Directory.CreateDirectory(Path.Combine(clientRoot, "scene"));
            Directory.CreateDirectory(Path.Combine(clientRoot, "environment"));
            Directory.CreateDirectory(Path.Combine(serverRoot, "logic"));
            Directory.CreateDirectory(Path.Combine(serverRoot, "collision"));

            var markers = UnityEngine.Object.FindObjectsOfType<MapLogicMarker>();
            var spawnJson = BuildSpawnPointsJson(markers);
            var npcJson = BuildNpcSpawnsJson(markers, mapId);
            var buildingsJson = BuildBuildingsJson();
            var lightingJson = BuildLightingJson();

            var spawnPath = "logic/spawn_points.json";
            var npcPath = "logic/npc_spawns.json";
            WriteSharedFile(clientRoot, spawnPath, spawnJson);
            WriteSharedFile(serverRoot, spawnPath, spawnJson);
            WriteSharedFile(clientRoot, npcPath, npcJson);
            WriteSharedFile(serverRoot, npcPath, npcJson);
            File.WriteAllText(Path.Combine(clientRoot, "scene", "buildings.json"), buildingsJson);
            File.WriteAllText(Path.Combine(clientRoot, "environment", "lighting.json"), lightingJson);

            var navPath = Path.Combine(serverRoot, "collision", "navmesh.bin");
            ExportNavMesh(navPath);

            var packageVersion = DateTime.UtcNow.ToString("yyyyMMdd") + ".1";
            var contentHash = ComputeContentHash(clientRoot, serverRoot);

            var manifestJson = BuildManifestJson(mapId, sceneName, packageVersion, contentHash, outRoot);
            File.WriteAllText(Path.Combine(clientRoot, "manifest.json"), manifestJson);
            File.WriteAllText(Path.Combine(serverRoot, "manifest.json"), manifestJson);

            var indexPath = Path.Combine(outRoot, "index.json");
            WriteIndexJson(indexPath, mapId, packageVersion, contentHash);

            Debug.LogFormat("MapExport：已导出 mapId={0} → {1}", mapId, outRoot);
        }

        private static void WriteSharedFile(string packageRoot, string relativePath, string json)
        {
            var full = Path.Combine(packageRoot, relativePath.Replace('/', Path.DirectorySeparatorChar));
            var dir = Path.GetDirectoryName(full);
            if (!string.IsNullOrEmpty(dir))
            {
                Directory.CreateDirectory(dir);
            }

            File.WriteAllText(full, json);
        }

        private static string BuildSpawnPointsJson(MapLogicMarker[] markers)
        {
            var sb = new StringBuilder();
            sb.Append("{\"spawns\":[");
            var first = true;
            foreach (var m in markers)
            {
                if (m.MarkerType != MapLogicMarkerType.SpawnPoint)
                {
                    continue;
                }

                if (!first)
                {
                    sb.Append(',');
                }

                first = false;
                var p = m.transform.position;
                sb.Append($"{{\"id\":\"{EscapeJson(m.MarkerId)}\",\"x\":{p.x:F3},\"y\":{p.y:F3},\"z\":{p.z:F3}}}");
            }

            sb.Append("]}");
            return sb.ToString();
        }

        private static string BuildNpcSpawnsJson(MapLogicMarker[] markers, int mapId)
        {
            var typeCounts = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
            foreach (var m in markers)
            {
                if (m.MarkerType != MapLogicMarkerType.NpcSpawn)
                {
                    continue;
                }

                var type = string.IsNullOrEmpty(m.MarkerId) ? "npc" : m.MarkerId;
                typeCounts.TryGetValue(type, out var count);
                typeCounts[type] = count + 1;
            }

            if (typeCounts.Count == 0)
            {
                var ambientPath = Path.Combine(
                    Path.GetFullPath(Path.Combine(Application.dataPath, "..", "Common", "map", mapId.ToString())),
                    "npc_spawns.json");
                if (!File.Exists(ambientPath))
                {
                    ambientPath = Path.Combine(
                        Path.GetFullPath(Path.Combine(Application.dataPath, "..", "Common", "map", mapId.ToString())),
                        "ambient.json");
                }

                if (File.Exists(ambientPath))
                {
                    return ConvertLegacyAmbientToNpcSpawns(File.ReadAllText(ambientPath));
                }
            }

            var sb = new StringBuilder();
            sb.Append("{\"npcs\":[");
            var first = true;
            foreach (var pair in typeCounts)
            {
                if (!first)
                {
                    sb.Append(',');
                }

                first = false;
                sb.Append($"{{\"type\":\"{EscapeJson(pair.Key)}\",\"count\":{pair.Value}}}");
            }

            sb.Append("]}");
            return sb.ToString();
        }

        private static string ConvertLegacyAmbientToNpcSpawns(string ambientJson)
        {
            var sb = new StringBuilder();
            sb.Append("{\"npcs\":[");
            var first = true;
            var content = ambientJson.Trim('{', '}', ' ', '\r', '\n', '\t');
            foreach (var pair in content.Split(','))
            {
                var colonIdx = pair.IndexOf(':');
                if (colonIdx < 0)
                {
                    continue;
                }

                var key = pair.Substring(0, colonIdx).Trim().Trim('"');
                var valStr = pair.Substring(colonIdx + 1).Trim();
                if (!int.TryParse(valStr, out var count) || count <= 0)
                {
                    continue;
                }

                if (!first)
                {
                    sb.Append(',');
                }

                first = false;
                sb.Append($"{{\"type\":\"{EscapeJson(key)}\",\"count\":{count}}}");
            }

            sb.Append("]}");
            return sb.ToString();
        }

        private static string BuildBuildingsJson()
        {
            return "{\"buildings\":[]}";
        }

        private static string BuildLightingJson()
        {
            var fog = RenderSettings.fog;
            var ambient = RenderSettings.ambientLight;
            return $"{{\"fog\":{(fog ? "true" : "false")},\"ambient\":[{ambient.r:F3},{ambient.g:F3},{ambient.b:F3},{ambient.a:F3}]}}";
        }

        private static string BuildManifestJson(int mapId, string sceneName, string packageVersion, string contentHash, string outRoot)
        {
            var baseUrl = "http://127.0.0.1:8765/rpg/map/";
            var sb = new StringBuilder();
            sb.AppendLine("{");
            sb.AppendLine($"  \"version\": {ManifestVersion},");
            sb.AppendLine($"  \"mapId\": {mapId},");
            sb.AppendLine($"  \"packageVersion\": \"{packageVersion}\",");
            sb.AppendLine($"  \"sceneName\": \"{EscapeJson(sceneName)}\",");
            sb.AppendLine($"  \"coordSystem\": \"{CoordSystem}\",");
            sb.AppendLine("  \"cdn\": {");
            sb.AppendLine($"    \"baseUrl\": \"{baseUrl}\",");
            sb.AppendLine($"    \"clientManifest\": \"client/{mapId}/manifest.json\",");
            sb.AppendLine($"    \"serverManifest\": \"server/{mapId}/manifest.json\",");
            sb.AppendLine($"    \"addressablesCatalog\": \"addressables/Map_{mapId}/catalog_1.0.json\"");
            sb.AppendLine("  },");
            sb.AppendLine("  \"assets\": {");
            sb.AppendLine($"    \"scene\": \"Addressables/Map_{mapId}/scene\",");
            sb.AppendLine($"    \"terrain\": \"Addressables/Map_{mapId}/terrain\",");
            sb.AppendLine($"    \"buildings\": \"Addressables/Map_{mapId}/buildings\"");
            sb.AppendLine("  },");
            sb.AppendLine("  \"sharedLogic\": {");
            sb.AppendLine("    \"spawnPoints\": \"logic/spawn_points.json\",");
            sb.AppendLine("    \"npcSpawns\": \"logic/npc_spawns.json\"");
            sb.AppendLine("  },");
            sb.AppendLine("  \"serverCollision\": \"collision/navmesh.bin\",");
            sb.AppendLine($"  \"contentHash\": \"{contentHash}\"");
            sb.AppendLine("}");
            return sb.ToString();
        }

        private static void WriteIndexJson(string indexPath, int mapId, string packageVersion, string contentHash)
        {
            var sb = new StringBuilder();
            sb.AppendLine("{");
            sb.AppendLine("  \"version\": 1,");
            sb.AppendLine("  \"maps\": [");
            sb.AppendLine("    {");
            sb.AppendLine($"      \"mapId\": {mapId},");
            sb.AppendLine($"      \"packageVersion\": \"{packageVersion}\",");
            sb.AppendLine($"      \"contentHash\": \"{contentHash}\",");
            sb.AppendLine($"      \"clientManifest\": \"client/{mapId}/manifest.json\",");
            sb.AppendLine($"      \"serverManifest\": \"server/{mapId}/manifest.json\"");
            sb.AppendLine("    }");
            sb.AppendLine("  ]");
            sb.AppendLine("}");
            File.WriteAllText(indexPath, sb.ToString());
        }

        private static void ExportNavMesh(string outputPath)
        {
            var triangulation = NavMesh.CalculateTriangulation();
            using (var stream = File.Create(outputPath))
            using (var writer = new BinaryWriter(stream))
            {
                writer.Write(Encoding.ASCII.GetBytes(NavMeshMagic));
                writer.Write(1);
                writer.Write(triangulation.vertices.Length);
                writer.Write(triangulation.indices.Length);
                foreach (var v in triangulation.vertices)
                {
                    writer.Write(v.x);
                    writer.Write(v.y);
                    writer.Write(v.z);
                }

                foreach (var index in triangulation.indices)
                {
                    writer.Write(index);
                }
            }
        }

        private static string ComputeContentHash(string clientRoot, string serverRoot)
        {
            using (var sha = SHA256.Create())
            {
                var files = new List<string>();
                files.AddRange(Directory.GetFiles(clientRoot, "*", SearchOption.AllDirectories));
                files.AddRange(Directory.GetFiles(serverRoot, "*", SearchOption.AllDirectories));
                files.Sort(StringComparer.OrdinalIgnoreCase);

                using (var ms = new MemoryStream())
                {
                    foreach (var file in files)
                    {
                        var bytes = File.ReadAllBytes(file);
                        ms.Write(bytes, 0, bytes.Length);
                    }

                    ms.Position = 0;
                    var hash = sha.ComputeHash(ms);
                    var sb = new StringBuilder("sha256:");
                    foreach (var b in hash)
                    {
                        sb.Append(b.ToString("x2"));
                    }

                    return sb.ToString();
                }
            }
        }

        private static int ParseMapId(string sceneName)
        {
            if (sceneName.StartsWith("World_") && int.TryParse(sceneName.Substring(6), out var id))
            {
                return id;
            }

            return 1002;
        }

        private static string EscapeJson(string value) =>
            string.IsNullOrEmpty(value) ? string.Empty : value.Replace("\\", "\\\\").Replace("\"", "\\\"");
    }
}
