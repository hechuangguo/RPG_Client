/// <summary>
/// 客户端功能模块号（对标 Common/ClientTypes.h ClientModule）。
/// </summary>
namespace Rpg.Client.Net
{
    public enum ClientModule : byte
    {
        Login = 0x00,
        Scene = 0x01,
        Battle = 0x02,
        Bag = 0x03,
        Skill = 0x04,
        Chat = 0x05,
        Social = 0x06,
        Quest = 0x07,
        Npc = 0x08,
        System = 0x0F
    }
}
