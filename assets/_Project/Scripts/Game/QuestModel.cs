/// <summary>
/// 任务运行时状态。
/// </summary>
using System.Collections.Generic;

namespace Rpg.Client.Game
{
    public sealed class QuestEntry
    {
        public uint QuestId;
        public string Name = string.Empty;
        public uint Progress;
        public uint Target;
        public bool Done;
    }

    public sealed class QuestModel
    {
        private readonly Dictionary<uint, QuestEntry> _entries = new Dictionary<uint, QuestEntry>();

        public IReadOnlyDictionary<uint, QuestEntry> Entries => _entries;

        public void Clear() => _entries.Clear();

        public void Upsert(QuestEntry entry)
        {
            if (entry == null)
            {
                return;
            }

            _entries[entry.QuestId] = entry;
        }
    }
}
