/// <summary>
/// CDN 地图包 manifest.json v1 数据结构（客户端/服务端共用字段）。
/// </summary>
using System;

namespace Rpg.Client.World
{
    [Serializable]
    public sealed class MapPackageCdn
    {
        public string baseUrl = string.Empty;
        public string clientManifest = string.Empty;
        public string serverManifest = string.Empty;
        public string addressablesCatalog = string.Empty;
    }

    [Serializable]
    public sealed class MapPackageAssets
    {
        public string scene = string.Empty;
        public string terrain = string.Empty;
        public string buildings = string.Empty;
    }

    [Serializable]
    public sealed class MapPackageSharedLogic
    {
        public string spawnPoints = string.Empty;
        public string npcSpawns = string.Empty;
    }

    [Serializable]
    public sealed class MapPackageManifest
    {
        public int version;
        public int mapId;
        public string packageVersion = string.Empty;
        public string sceneName = string.Empty;
        public string coordSystem = string.Empty;
        public MapPackageCdn cdn;
        public MapPackageAssets assets;
        public MapPackageSharedLogic sharedLogic;
        public string serverCollision = string.Empty;
        public string contentHash = string.Empty;

        public bool HasRemoteScene =>
            assets != null && !string.IsNullOrEmpty(assets.scene);
    }
}
