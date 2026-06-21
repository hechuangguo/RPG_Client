/// <summary>
/// 启动入口（对标 main.cpp）。
/// 职责：初始化 ClientLogger；GameApp 由 Boot 场景挂载。
/// </summary>
using Rpg.Client.Log;
using UnityEngine;

namespace Rpg.Client.App
{
    public sealed class GameBootstrap : MonoBehaviour
    {
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.BeforeSceneLoad)]
        private static void OnBeforeSceneLoad()
        {
            ClientLogger.Instance.Initialize();
            ClientLogger.Instance.Info("GameBootstrap：Unity 客户端启动");
        }
    }
}
