EventBus = EventBus or { _handlers = {} }

function EventBus.on(event, fn)
    if not EventBus._handlers[event] then EventBus._handlers[event] = {} end
    table.insert(EventBus._handlers[event], fn)
end

function EventBus.fire(event, ...)
    local list = EventBus._handlers[event]
    if not list then return end
    for _, fn in ipairs(list) do fn(...) end
end

return EventBus
