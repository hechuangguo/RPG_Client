/// <summary>
/// 背包运行时状态。
/// 支持全量同步 SetSlots 和增量操作 AddItem / UpdateItemCount / RemoveItem。
/// </summary>
using System;
using System.Collections.Generic;

namespace Rpg.Client.Game
{
    public sealed class BagSlotEntry
    {
        public uint ItemId;
        public uint Count;
    }

    public sealed class ItemBagModel
    {
        private readonly List<BagSlotEntry> _slots = new List<BagSlotEntry>();

        /// <summary>最大背包容量。0 表示不限制（当前未实现容量检查逻辑）。</summary>
        public uint Capacity { get; set; }

        public IReadOnlyList<BagSlotEntry> Slots => _slots;

        /// <summary>数据变更时触发，UI 可订阅以自动刷新。</summary>
        public event Action OnChanged;

        public void SetSlots(IEnumerable<BagSlotEntry> slots)
        {
            _slots.Clear();
            if (slots == null)
            {
                OnChanged?.Invoke();
                return;
            }

            foreach (var slot in slots)
            {
                if (slot != null && slot.ItemId > 0)
                {
                    _slots.Add(slot);
                }
            }

            OnChanged?.Invoke();
        }

        /// <summary>添加物品（如物品已存在则累加数量）。</summary>
        public void AddItem(uint itemId, uint count)
        {
            if (itemId == 0 || count == 0)
            {
                return;
            }

            var existing = FindSlot(itemId);
            if (existing != null)
            {
                existing.Count += count;
            }
            else
            {
                _slots.Add(new BagSlotEntry { ItemId = itemId, Count = count });
            }

            OnChanged?.Invoke();
        }

        /// <summary>更新物品数量（若变为 0 则自动移除）。</summary>
        public void UpdateItemCount(uint itemId, uint newCount)
        {
            var existing = FindSlot(itemId);
            if (existing == null)
            {
                if (newCount > 0)
                {
                    _slots.Add(new BagSlotEntry { ItemId = itemId, Count = newCount });
                    OnChanged?.Invoke();
                }

                return;
            }

            if (newCount == 0)
            {
                _slots.Remove(existing);
            }
            else
            {
                existing.Count = newCount;
            }

            OnChanged?.Invoke();
        }

        /// <summary>移除物品。</summary>
        public bool RemoveItem(uint itemId)
        {
            return RemoveItem(itemId, 1);
        }

        /// <summary>移除指定数量的物品。</summary>
        public bool RemoveItem(uint itemId, uint count)
        {
            var existing = FindSlot(itemId);
            if (existing == null || existing.Count < count)
            {
                return false;
            }

            existing.Count -= count;
            if (existing.Count == 0)
            {
                _slots.Remove(existing);
            }

            OnChanged?.Invoke();
            return true;
        }

        /// <summary>查找指定物品的背包格子。</summary>
        private BagSlotEntry FindSlot(uint itemId)
        {
            foreach (var slot in _slots)
            {
                if (slot.ItemId == itemId)
                {
                    return slot;
                }
            }

            return null;
        }
    }
}
