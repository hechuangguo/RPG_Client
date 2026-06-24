/// <summary>
/// 跨平台退出客户端（编辑器内停止 Play，发布包调用 Application.Quit）。
/// </summary>
using UnityEngine;

namespace Rpg.Client.Util
{
    public static class ClientApplication
    {
        public static void RequestQuit()
        {
#if UNITY_EDITOR
            UnityEditor.EditorApplication.isPlaying = false;
#else
            Application.Quit();
#endif
        }
    }
}
