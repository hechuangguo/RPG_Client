/// <summary>
/// World_{mapId} 场景一键生成：地形、出生点、NavMesh、定向光。
/// 菜单：RPG → Setup World Scene 1002
/// </summary>
using System.IO;
using Rpg.Client.World;
using Unity.AI.Navigation;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace Rpg.Client.EditorTools
{
    public static class WorldSceneSetup
    {
        private const int DefaultMapId = 1002;
        private const string ScenePath = "assets/_Project/Scenes/World_1002.unity";

        [MenuItem("RPG/Setup World Scene 1002")]
        public static void SetupWorldScene1002()
        {
            if (!GuardNotPlaying("创建 World 场景"))
            {
                return;
            }

            SetupWorldSceneInternal(DefaultMapId, forceOverwrite: !File.Exists(ScenePath) ||
                EditorUtility.DisplayDialog("Setup World Scene", "World_1002.unity 已存在，是否覆盖？", "覆盖", "取消"));
        }

        /// <summary>batchmode：-executeMethod Rpg.Client.EditorTools.WorldSceneSetup.SetupWorldScene1002Batch</summary>
        public static void SetupWorldScene1002Batch() => SetupWorldSceneInternal(DefaultMapId, forceOverwrite: true);

        private static bool GuardNotPlaying(string action)
        {
            if (!EditorApplication.isPlaying && !EditorApplication.isPlayingOrWillChangePlaymode)
            {
                return true;
            }

            Debug.LogWarningFormat("WorldSceneSetup：Play 模式下无法{0}，请先停止运行", action);
            return false;
        }

        private static void SetupWorldSceneInternal(int mapId, bool forceOverwrite)
        {
            if (!forceOverwrite && File.Exists(ScenePath))
            {
                return;
            }

            var sceneDir = Path.GetDirectoryName(ScenePath);
            if (!string.IsNullOrEmpty(sceneDir))
            {
                Directory.CreateDirectory(sceneDir);
            }

            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            scene.name = $"World_{mapId}";

            var worldRoot = new GameObject("WorldRoot");
            var terrain = GameObject.CreatePrimitive(PrimitiveType.Plane);
            terrain.name = "Terrain";
            terrain.transform.SetParent(worldRoot.transform, false);
            terrain.transform.localScale = new Vector3(4f, 1f, 4f);

            var spawnGo = new GameObject("Spawn_Default");
            spawnGo.transform.SetParent(worldRoot.transform, false);
            spawnGo.transform.position = new Vector3(0f, 0f, 0f);
            var marker = spawnGo.AddComponent<MapLogicMarker>();
            marker.MarkerType = MapLogicMarkerType.SpawnPoint;
            marker.MarkerId = "default";

            var lightGo = new GameObject("Directional Light");
            var light = lightGo.AddComponent<Light>();
            light.type = LightType.Directional;
            light.color = new Color(1f, 0.96f, 0.84f);
            light.intensity = 1.1f;
            lightGo.transform.rotation = Quaternion.Euler(50f, -30f, 0f);

            var navSurface = worldRoot.AddComponent<NavMeshSurface>();
            navSurface.collectObjects = CollectObjects.All;
            navSurface.BuildNavMesh();

            EditorSceneManager.SaveScene(scene, ScenePath);
            AddSceneToBuildSettings(ScenePath);

            Debug.LogFormat("WorldSceneSetup：已创建 {0}（mapId={1}）", ScenePath, mapId);
        }

        private static void AddSceneToBuildSettings(string scenePath)
        {
            var scenes = new System.Collections.Generic.List<EditorBuildSettingsScene>(EditorBuildSettings.scenes);
            foreach (var existing in scenes)
            {
                if (existing.path == scenePath)
                {
                    return;
                }
            }

            scenes.Add(new EditorBuildSettingsScene(scenePath, true));
            EditorBuildSettings.scenes = scenes.ToArray();
        }
    }
}
