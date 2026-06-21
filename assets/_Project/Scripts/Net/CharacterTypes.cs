/// <summary>
/// 角色列表条目（对标 net/CharacterTypes.h CharacterEntry）。
/// </summary>
namespace Rpg.Client.Net
{
    public static class CharacterDef
    {
        public const byte VocationWarrior = 0;
        public const byte VocationMage = 1;
        public const byte SexMale = 0;
        public const byte SexFemale = 1;
    }

    public sealed class CharacterEntry
    {
        public ulong UserId;
        public string Name = string.Empty;
        public uint Level;
        public byte Vocation;
        public byte Sex;
    }
}
