/// <summary>
/// 角色列表条目与职业/性别常量。
/// </summary>
namespace Rpg.Client.Net
{
    public static class CharacterDef
    {
        public const byte VocationWarrior = 0;
        public const byte VocationMage = 1;
        public const byte SexMale = 0;
        public const byte SexFemale = 1;

        /// <summary>角色模型ID常量。</summary>
        public const uint ModelMaleAdult = 1;
        public const uint ModelMaleChild = 2;
        public const uint ModelFemaleAdult = 3;
        public const uint ModelFemaleChild = 4;
        /// <summary>默认模型：男·大人。</summary>
        public const uint ModelDefault = ModelMaleAdult;
    }

    public sealed class CharacterEntry
    {
        public ulong UserId;
        public string Name = string.Empty;
        public uint Level;
        public byte Vocation;
        public byte Sex;
        public uint ModelId = CharacterDef.ModelDefault;
    }
}
