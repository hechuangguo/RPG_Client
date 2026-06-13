/**
 * @file    QuestModel.h
 * @brief   客户端任务数据模型
 *
 * 职责：
 * - 缓存当前追踪任务列表
 * - 供 GameScene HUD 与 Lua 读取
 *
 * 协作：ClientScriptHost、GameScene。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief 单条任务追踪信息
 */
struct QuestEntry
{
    uint32_t    questId;   /**< 任务 ID */
    std::string name;      /**< 任务名 */
    uint32_t    progress;  /**< 当前进度 */
    uint32_t    target;    /**< 目标数量 */
    bool        done;      /**< 是否已完成待提交 */
};

/**
 * @brief 任务模型
 */
class QuestModel
{
public:
    QuestModel();

    /** @brief 清空全部任务 */
    void clear();

    /**
     * @brief 更新或插入任务
     * @param entry 任务条目
     */
    void upsert(const QuestEntry& entry);

    /** @brief 全部任务 */
    const std::vector<QuestEntry>& quests() const;

    /**
     * @brief 获取第一条追踪任务摘要（HUD 用）
     * @return 摘要字符串，无任务时返回空
     */
    std::string primarySummary() const;

private:
    std::vector<QuestEntry> m_quests;
};
