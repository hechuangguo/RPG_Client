/// <summary>
/// 客户端顶层应用状态枚举。
/// </summary>
namespace Rpg.Client.App
{
    public enum AppState
    {
        ZoneHome,
        ServerList,
        LoadingAuth,
        AuthLogin,
        Register,
        Connecting,
        CharacterSelect,
        Game
    }
}
