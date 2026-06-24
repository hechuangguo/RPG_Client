-- Phase 3 预留：C# GameScriptHost 接入 XLua 后由 init.lua 统一分发事件。
EventBus = EventBus or { _handlers = {} }

function EventBus.on(event, fn)
    if not EventBus._handlers[event] then EventBus._handlers[event] = {} end
    table.insert(EventBus._handlers[event], fn)
end

--- 取消订阅指定事件的指定回调。
function EventBus.off(event, fn)
    local list = EventBus._handlers[event]
    if not list then return end
    -- 倒序遍历，安全删除
    for i = #list, 1, -1 do
        if list[i] == fn then
            table.remove(list, i)
        end
    end
    if #list == 0 then
        EventBus._handlers[event] = nil
    end
end

--- 触发事件。先预拷贝 handler 列表，防止回调中 on/off 导致遍历行为未定义。
function EventBus.fire(event, ...)
    local list = EventBus._handlers[event]
    if not list then return end
    local copy = {}
    for i = 1, #list do
        copy[i] = list[i]
    end
    for _, fn in ipairs(copy) do
        fn(...)
    end
end

return EventBus
