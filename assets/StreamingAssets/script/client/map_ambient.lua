-- MapAmbient: 地图静态 NPC 管理
-- 由 C# WorldController.LoadMap 触发，读取 map/{mapId}/ambient.json 并生成 NPC。
-- NPC 生成与 AI 驱动由 C# 侧 MapAmbientController 完成，Lua 侧仅负责桥接与配置校验。

MapAmbient = {}

--- 进入地图时回调。C# MapAmbientController.LoadAmbient 已处理 NPC 生成。
--- 此处可用于 Lua 侧的额外配置（如自定义 NPC 对话脚本）。
--- @param mapId number 地图 ID
function MapAmbient.onEnter(mapId)
    if log_info then
        log_info("MapAmbient: onEnter map " .. tostring(mapId))
    end

    -- Phase 2: 加载地图特定的 NPC 对话树 / 脚本
    local script_path = "map/" .. tostring(mapId) .. "/npc_script.lua"
    -- if file_exists(script_path) then dofile(script_path) end
end

--- 每帧驱动（由 C# GameScriptHost.OnTick 转发）。
--- @param nowMs number 当前时间戳（毫秒）
function MapAmbient.update(nowMs)
    -- Phase 2: 驱动 NPC 对话状态机 / 巡逻路径
end

return MapAmbient
