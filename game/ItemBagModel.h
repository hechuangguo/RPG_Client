/**
 * @file    ItemBagModel.h
 * @brief   客户端背包数据模型
 *
 * 职责：
 * - 缓存背包槽位 itemId/count
 * - 供 Lua 与 HUD 查询
 *
 * 协作：ClientScriptHost。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <cstdint>
#include <vector>

/**
 * @brief 背包单格
 */
struct BagSlot
{
    uint32_t itemId; /**< 道具配置 ID，0 表示空 */
    uint32_t count;  /**< 数量 */
};

/**
 * @brief 背包模型
 */
class ItemBagModel
{
public:
    ItemBagModel();

    /** @brief 清空背包 */
    void clear();

    /**
     * @brief 设置全部槽位
     * @param slots 槽位数组
     */
    void setSlots(const std::vector<BagSlot>& slots);

    /** @brief 当前槽位 */
    const std::vector<BagSlot>& slots() const;

    /**
     * @brief 统计某道具总数
     * @param itemId 道具 ID
     * @return 总数量
     */
    uint32_t countItem(uint32_t itemId) const;

private:
    std::vector<BagSlot> m_slots;
};
