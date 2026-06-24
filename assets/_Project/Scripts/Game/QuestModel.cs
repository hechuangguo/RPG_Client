/// <summary>
/// 任务运行时状态。
/// </summary>
using System;
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

        /// <summary>数据变更时触发，UI 可订阅以自动刷新。</summary>
        public event Action OnChanged;

        public void Clear()
        {
            _entries.Clear();
            OnChanged?.Invoke();
        }

        public void Upsert(QuestEntry entry)
        {
            if (entry == null)
            {
                return;
            }

            _entries[entry.QuestId] = entry;
            OnChanged?.Invoke();
        }

        public void ReplaceAll(IEnumerable<QuestEntry> entries)
        {
            _entries.Clear();
            if (entries != null)
            {
                foreach (var entry in entries)
                {
                    if (entry == null || entry.QuestId == 0)
                    {
                        continue;
                    }

                    _entries[entry.QuestId] = entry;
                }
            }

            OnChanged?.Invoke();
        }

        /// <summary>移除指定任务（任务完成/放弃后调用）。</summary>
        public bool Remove(uint questId)
        {
            if (_entries.Remove(questId))
            {
                OnChanged?.Invoke();
                return true;
            }

            return false;
        }

        /// <summary>按 QuestId 查找任务实体。</summary>
        public bool TryGetEntry(uint questId, out QuestEntry entry)
        {
            return _entries.TryGetValue(questId, out entry);
        }
    }
}
