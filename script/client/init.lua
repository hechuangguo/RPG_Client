-- 客户端 Lua 入口（Phase 3 设计稿；运行时由 C# GameScriptHost 驱动，尚未接入 VM）
-- 真源目录：script/client/；构建前由 sync_streaming_assets.ps1 同步至 StreamingAssets/script/
require("client.quest_client")
require("client.item_client")
require("client.map_ambient")

function OnClientInit()
    if log_info then log_info("Client Lua init") end
end

function OnEnterGame(userId, mapId)
    if log_info then log_info(string.format("EnterGame user=%s map=%s", tostring(userId), tostring(mapId))) end
    MapAmbient.onEnter(mapId)
end

function OnTick(nowMs)
    MapAmbient.update(nowMs)
end

function OnQuestInfo(questId, name, progress, target, done)
    QuestClient.onInfo(questId, name, progress, target, done)
end

function OnBagInfo(slots)
    local map = {}
    if slots then
        for _, s in ipairs(slots) do
            if s.itemId and s.itemId > 0 then
                map[s.itemId] = s.count or 0
            end
        end
    end
    if ItemClient and ItemClient.onBagSync then
        ItemClient.onBagSync(map)
    end
end

function OnChat(fromId, fromName, channel, content)
    if log_info then
        log_info(string.format("[Chat ch=%s] %s: %s", tostring(channel), fromName or "", content or ""))
    end
end

function OnNotice(content)
    if log_info then log_info("[Notice] " .. tostring(content)) end
end
