require("client.event_bus")
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

function OnBagInfo(json)
    -- placeholder
end
