/// <summary>
/// 地图资源加载：CDN manifest → Addressables Catalog → 场景；失败时回退 Common/map。
/// </summary>
using System;
using System.Collections;
using System.IO;
using Rpg.Client.Config;
using Rpg.Client.Log;
using UnityEngine;
using UnityEngine.AddressableAssets;
using UnityEngine.Networking;
using UnityEngine.ResourceManagement.AsyncOperations;
using UnityEngine.ResourceManagement.ResourceProviders;
using UnityEngine.SceneManagement;

namespace Rpg.Client.World
{
    public sealed class MapLoadResult
    {
        public MapPackageManifest Manifest;
        public MapData LocalData;
        public bool UsedAddressables;
        public string SceneKey = string.Empty;
        public string ScenePath = string.Empty;
        public AsyncOperationHandle<SceneInstance> SceneHandle;
    }

    public static class MapLoader
    {
        private const string CoordSystem = "unity_y_up";

        public static IEnumerator LoadAsync(
            uint mapId,
            ClientConfigLoader config,
            Action<MapLoadResult> onReady,
            Action<string> onError)
        {
            if (config == null)
            {
                onError?.Invoke("配置未加载");
                yield break;
            }

            var result = new MapLoadResult
            {
                LocalData = MapDataLoader.Load(mapId)
            };

            MapPackageManifest manifest = null;
            var cdnBase = NormalizeBaseUrl(config.MapCdnBaseUrl);
            if (!string.IsNullOrEmpty(cdnBase))
            {
                yield return FetchManifest(cdnBase, mapId, m => manifest = m, onError);
            }

            if (manifest == null && !string.IsNullOrEmpty(cdnBase))
            {
                ClientLogger.Instance.WarnFormat("MapLoader：CDN manifest 不可用，回退本地 map/{0}", mapId);
            }

            result.Manifest = manifest;

            if (manifest != null && manifest.HasRemoteScene)
            {
                var catalogUrl = ResolveCatalogUrl(config, manifest);
                if (!string.IsNullOrEmpty(catalogUrl))
                {
                    ClientLogger.Instance.InfoFormat("MapLoader：更新 Addressables Catalog {0}", catalogUrl);
                    var catalogHandle = Addressables.LoadContentCatalogAsync(catalogUrl);
                    yield return catalogHandle;
                    if (catalogHandle.Status != AsyncOperationStatus.Succeeded)
                    {
                        onError?.Invoke("Addressables Catalog 加载失败");
                        Addressables.Release(catalogHandle);
                        yield break;
                    }

                    Addressables.Release(catalogHandle);
                }

                result.UsedAddressables = true;
                result.SceneKey = manifest.assets.scene;
                ClientLogger.Instance.InfoFormat(
                    "MapLoader：Addressables 加载场景 {0}（包版本 {1}）",
                    result.SceneKey,
                    manifest.packageVersion);

                var sceneHandle = Addressables.LoadSceneAsync(result.SceneKey, LoadSceneMode.Additive);
                yield return sceneHandle;
                if (sceneHandle.Status != AsyncOperationStatus.Succeeded)
                {
                    onError?.Invoke($"场景加载失败：{result.SceneKey}");
                    yield break;
                }

                result.SceneHandle = sceneHandle;
                onReady?.Invoke(result);
                yield break;
            }

            var scenePath = result.LocalData?.Meta?.scenePath;
            if (string.IsNullOrEmpty(scenePath))
            {
                onError?.Invoke($"地图 {mapId} 无可用场景路径");
                yield break;
            }

            result.ScenePath = scenePath;
            ClientLogger.Instance.InfoFormat("MapLoader：SceneManager 附加加载 {0}", scenePath);
            var op = SceneManager.LoadSceneAsync(scenePath, LoadSceneMode.Additive);
            while (op != null && !op.isDone)
            {
                yield return null;
            }

            onReady?.Invoke(result);
        }

        public static void Unload(MapLoadResult result)
        {
            if (result == null)
            {
                return;
            }

            if (result.UsedAddressables && result.SceneHandle.IsValid())
            {
                Addressables.UnloadSceneAsync(result.SceneHandle);
                return;
            }

            if (!string.IsNullOrEmpty(result.ScenePath))
            {
                SceneManager.UnloadSceneAsync(result.ScenePath);
            }
        }

        private static IEnumerator FetchManifest(
            string cdnBase,
            uint mapId,
            Action<MapPackageManifest> onSuccess,
            Action<string> onError)
        {
            var url = $"{cdnBase}client/{mapId}/manifest.json";
            using (var req = UnityWebRequest.Get(url))
            {
                req.timeout = 15;
                yield return req.SendWebRequest();
#if UNITY_2020_1_OR_NEWER
                if (req.result != UnityWebRequest.Result.Success)
#else
                if (req.isNetworkError || req.isHttpError)
#endif
                {
                    ClientLogger.Instance.WarnFormat("MapLoader：拉取 manifest 失败 {0} — {1}", url, req.error);
                    yield break;
                }

                try
                {
                    var manifest = JsonUtility.FromJson<MapPackageManifest>(req.downloadHandler.text);
                    if (manifest == null || manifest.mapId <= 0)
                    {
                        onError?.Invoke("manifest 解析失败");
                        yield break;
                    }

                    onSuccess?.Invoke(manifest);
                }
                catch (Exception ex)
                {
                    onError?.Invoke($"manifest 解析异常：{ex.Message}");
                }
            }
        }

        private static string ResolveCatalogUrl(ClientConfigLoader config, MapPackageManifest manifest)
        {
            if (!string.IsNullOrEmpty(config.AddressablesRemoteUrl))
            {
                return config.AddressablesRemoteUrl;
            }

            if (manifest.cdn == null || string.IsNullOrEmpty(manifest.cdn.addressablesCatalog))
            {
                return string.Empty;
            }

            var baseUrl = NormalizeBaseUrl(manifest.cdn.baseUrl);
            if (string.IsNullOrEmpty(baseUrl))
            {
                baseUrl = NormalizeBaseUrl(config.MapCdnBaseUrl);
            }

            return string.IsNullOrEmpty(baseUrl)
                ? manifest.cdn.addressablesCatalog
                : baseUrl + manifest.cdn.addressablesCatalog;
        }

        private static string NormalizeBaseUrl(string url)
        {
            if (string.IsNullOrWhiteSpace(url))
            {
                return string.Empty;
            }

            var trimmed = url.Trim();
            return trimmed.EndsWith("/", StringComparison.Ordinal) ? trimmed : trimmed + "/";
        }

        /// <summary>开发期：将 manifest 写入 StreamingAssets 便于离线测试。</summary>
        public static bool TryLoadLocalManifest(uint mapId, out MapPackageManifest manifest)
        {
            manifest = null;
            var path = Path.Combine(Application.streamingAssetsPath, "map", mapId.ToString(), "manifest.json");
            if (!File.Exists(path))
            {
                return false;
            }

            try
            {
                manifest = JsonUtility.FromJson<MapPackageManifest>(File.ReadAllText(path));
                return manifest != null;
            }
            catch
            {
                return false;
            }
        }
    }
}
