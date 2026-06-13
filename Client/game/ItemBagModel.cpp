/**
 * @file    ItemBagModel.cpp
 * @brief   ItemBagModel 实现
 */

#include "game/ItemBagModel.h"

ItemBagModel::ItemBagModel() = default;

void ItemBagModel::clear()
{
    m_slots.clear();
}

void ItemBagModel::setSlots(const std::vector<BagSlot>& slots)
{
    m_slots = slots;
}

const std::vector<BagSlot>& ItemBagModel::slots() const
{
    return m_slots;
}

uint32_t ItemBagModel::countItem(uint32_t itemId) const
{
    uint32_t total = 0;
    for (const BagSlot& s : m_slots)
    {
        if (s.itemId == itemId)
        {
            total += s.count;
        }
    }
    return total;
}
