/// <summary>
/// 地图逻辑标记（MapExport 导出 spawn/NPC/trigger）。
/// 放置于 Unity 场景空物体上。
/// </summary>
using UnityEngine;

namespace Rpg.Client.World
{
    public enum MapLogicMarkerType
    {
        SpawnPoint,
        NpcSpawn,
        Trigger
    }

    public sealed class MapLogicMarker : MonoBehaviour
    {
        public MapLogicMarkerType MarkerType = MapLogicMarkerType.SpawnPoint;
        public string MarkerId = string.Empty;
        public int NpcTemplateId;
        public float Radius = 1f;
    }
}
