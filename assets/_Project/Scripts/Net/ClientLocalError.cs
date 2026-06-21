/// <summary>
/// 客户端本地错误码。
/// </summary>
namespace Rpg.Client.Net
{
    public enum ClientLocalError
    {
        None = 0,
        ConnectTimeout,
        ResponseTimeout,
        ZoneListTimeout,
        LogoutTimeout,
        Disconnected,
        ParseError,
        GatewayError
    }

    public enum LoginTimeoutContext
    {
        Generic,
        Login,
        Register,
        Gateway,
        UserList,
        CreateCharacter,
        EnterGame,
        ZoneList,
        LogoutTimeout
    }
}
