/// <summary>
/// 背包运行时状态。
/// </summary>
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

        public IReadOnlyList<BagSlotEntry> Slots => _slots;

        public void SetSlots(IEnumerable<BagSlotEntry> slots)
        {
            _slots.Clear();
            if (slots == null)
            {
                return;
            }

            foreach (var slot in slots)
            {
                if (slot != null && slot.ItemId > 0)
                {
                    _slots.Add(slot);
                }
            }
        }
    }
}
