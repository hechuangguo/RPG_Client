QuestClient = {}

local _track = nil

function QuestClient.onInfo(questId, name, progress, target, done)
    _track = { questId = questId, name = name, progress = progress, target = target, done = done }
    if log_info then
        log_info(string.format("[Quest] %s %d/%d", name or "", progress or 0, target or 0))
    end
end

function QuestClient.getTrack()
    return _track
end

function QuestClient.requestAccept(questId)
    if send_quest_accept then send_quest_accept(questId) end
end

function QuestClient.requestSubmit(questId)
    if send_quest_submit then send_quest_submit(questId) end
end

return QuestClient
