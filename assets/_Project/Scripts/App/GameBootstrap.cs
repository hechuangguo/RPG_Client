/// <summary>
/// 启动入口（挂载 GameApp 的场景引导）。
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
