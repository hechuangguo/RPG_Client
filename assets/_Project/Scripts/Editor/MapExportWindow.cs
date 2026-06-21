/// <summary>
/// MapExport Editor 工具（计划 §五 MapExport）。
/// 菜单：RPG → Export Map Package
/// </summary>
using System.IO;
using Rpg.Client.World;
using UnityEditor;
using UnityEngine;

namespace Rpg.Client.EditorTools
{
    public static class MapExportWindow
    {
        private const int ManifestVersion = 1;

        [MenuItem("RPG/Export Map Package")]
        public static void ExportMapPackage()
        {
            var scene = UnityEngine.SceneManagement.SceneManager.GetActiveScene();
            if (!scene.IsValid())
            {
                EditorUtility.DisplayDialog("MapExport", "无有效场景", "确定");
                return;
            }

            var mapId = ParseMapId(scene.name);
            var outRoot = EditorUtility.OpenFolderPanel("选择导出目录", "map", "");
            if (string.IsNullOrEmpty(outRoot))
            {
                return;
            }

            var clientRoot = Path.Combine(outRoot, "client", mapId.ToString());
            Directory.CreateDirectory(clientRoot);

            var manifest = new MapManifestData
            {
                version = ManifestVersion,
                mapId = mapId,
                sceneName = scene.name
            };
            File.WriteAllText(Path.Combine(clientRoot, "manifest.json"), JsonUtility.ToJson(manifest, true));

            var markers = Object.FindObjectsOfType<MapLogicMarker>();
            var spawnJson = "{\"spawns\":[";
            for (var i = 0; i < markers.Length; i++)
            {
                var m = markers[i];
                if (m.MarkerType != MapLogicMarkerType.SpawnPoint)
                {
                    continue;
                }

                var p = m.transform.position;
                spawnJson += $"{{\"id\":\"{m.MarkerId}\",\"x\":{p.x},\"y\":{p.y},\"z\":{p.z}}}";
                if (i < markers.Length - 1)
                {
                    spawnJson += ",";
                }
            }

            spawnJson += "]}";
            Directory.CreateDirectory(Path.Combine(clientRoot, "logic"));
            File.WriteAllText(Path.Combine(clientRoot, "logic", "spawn_points.json"), spawnJson);

            EditorUtility.DisplayDialog("MapExport", $"已导出 client 包至\n{clientRoot}", "确定");
        }

        private static int ParseMapId(string sceneName)
        {
            if (sceneName.StartsWith("World_") && int.TryParse(sceneName.Substring(6), out var id))
            {
                return id;
            }

            return 1002;
        }

        [System.Serializable]
        private sealed class MapManifestData
        {
            public int version;
            public int mapId;
            public string sceneName;
        }
    }
}
