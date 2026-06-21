/// <summary>
/// 客户端顶层应用状态（对标 app/AppState.h）。
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
