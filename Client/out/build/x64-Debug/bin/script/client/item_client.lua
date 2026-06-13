ItemClient = {}

local _slots = {}

function ItemClient.onBagSync(slots)
    _slots = slots or {}
end

function ItemClient.addItem(itemId, count)
    _slots[itemId] = (_slots[itemId] or 0) + (count or 1)
    local cfg = DataTable.load("item_config")
    local row = cfg and DataTable.getById(cfg, itemId)
    local name = row and row.name or tostring(itemId)
    if log_info then log_info(string.format("获得 %s x%d", name, count or 1)) end
end

function ItemClient.getItemName(itemId)
    local cfg = DataTable.load("item_config")
    local row = cfg and DataTable.getById(cfg, itemId)
    return row and row.name or ("物品" .. tostring(itemId))
end

return ItemClient
