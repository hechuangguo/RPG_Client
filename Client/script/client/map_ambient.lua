MapAmbient = {}

function MapAmbient.onEnter(mapId)
    if log_info then log_info("MapAmbient enter map " .. tostring(mapId)) end
end

function MapAmbient.update(nowMs)
end

return MapAmbient
