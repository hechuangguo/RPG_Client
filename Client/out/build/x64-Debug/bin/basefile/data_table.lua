-- basefile/data_table.lua — 策划表加载与查询
DataTable = DataTable or {}
local _cache = {}

function DataTable.load(moduleName)
    if not moduleName or moduleName == "" then return nil end
    if _cache[moduleName] then return _cache[moduleName] end
    local ok, data = pcall(require, moduleName)
    if not ok or type(data) ~= "table" then
        if log_info then
            log_info(string.format("[DataTable] load failed: %s (%s)", tostring(moduleName), tostring(data)))
        end
        return nil
    end
    _cache[moduleName] = data
    return data
end

function DataTable.getById(tbl, id)
    if not tbl or id == nil then return nil end
    return tbl[id]
end

function DataTable.forEach(tbl, fn)
    if not tbl or type(fn) ~= "function" then return end
    for id, row in pairs(tbl) do
        if fn(id, row) == false then break end
    end
end

function DataTable.filter(tbl, fieldName, fieldValue)
    local out = {}
    if not tbl then return out end
    for id, row in pairs(tbl) do
        if row[fieldName] == fieldValue then out[#out + 1] = row end
    end
    return out
end

function DataTable.clearCache()
    for name in pairs(_cache) do
        package.loaded[name] = nil
        _cache[name] = nil
    end
end

return DataTable
