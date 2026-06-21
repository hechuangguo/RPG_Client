/// <summary>
/// Lua 脚本宿主占位（对标 lua/ClientScriptHost）。
/// Phase 3：评估 XLua 或 C# 移植，见 docs/LUA_BRIDGE.md。
/// </summary>
namespace Rpg.Client.Scripting
{
    /// <summary>未来绑定 script/client/*.lua；当前未启用。</summary>
    public sealed class LuaScriptHost
    {
        public bool Initialize(string scriptRoot) => false;
        public void Shutdown() { }
        public void OnQuestUpdate(uint questId) { }
    }
}
