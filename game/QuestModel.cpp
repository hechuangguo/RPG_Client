/**
 * @file    QuestModel.cpp
 * @brief   QuestModel 实现
 */

#include "game/QuestModel.h"

QuestModel::QuestModel() = default;

void QuestModel::clear()
{
    m_quests.clear();
}

void QuestModel::upsert(const QuestEntry& entry)
{
    for (auto& q : m_quests)
    {
        if (q.questId == entry.questId)
        {
            q = entry;
            return;
        }
    }
    m_quests.push_back(entry);
}

const std::vector<QuestEntry>& QuestModel::quests() const
{
    return m_quests;
}

std::string QuestModel::primarySummary() const
{
    if (m_quests.empty())
    {
        return u8"暂无任务";
    }
    const QuestEntry& q = m_quests.front();
    return q.name + " (" + std::to_string(q.progress) + "/" + std::to_string(q.target) + ")";
}
