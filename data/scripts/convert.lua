import "util"

ability {
    name = "convert",
    run = function(ability)
        local unit = ability:getUnit()
        local properties = ability:getProperties()
        -- See whether we're close enough.
        if util.distanceToJob(unit) <= 0.01 then
            -- In range, do our thing.
            unit:getJob():getTarget():convert(unit:getOwner(), properties.strength)
            return properties.cooldown
        else
            -- Not in range, report failure.
            return false
        end
    end
}

--[[
-----------------------------------------------------------
Floor conversion.
-----------------------------------------------------------
]]--

local function checkJob(player, block, x, y)
    if block:isConvertible() and block:isPassable() then
        if block:getOwner() ~= player then
            if util.isFloorOwnedBy(Block.at(x - 1, y), player) or
                util.isFloorOwnedBy(Block.at(x + 1, y), player) or
                util.isFloorOwnedBy(Block.at(x, y - 1), player) or
                util.isFloorOwnedBy(Block.at(x, y + 1), player)
            then
                if not block:isTargetOfJobByType(player, "convert_floor") then
                    Job.create {name="convert_floor", player=player, target=block}
                end
                return
            end
        -- elseif block:isDamaged() then
            -- TODO allow repairs
        end
    end
    -- Fail case -- not valid for being converted.
    Job.deleteByTypeWhereTarget(player, "convert_floor", block)
end

--[[
Used for both type and owner changes. Checks self and neighboring tiles for
validity and creates / deletes jobs as necessary.
--]]
local function convertFloorBlockChangeHandler(block)
    local x, y = block:getPosition()
    for player = 1, 4 do
        checkJob(player, block, x, y)
        checkJob(player, Block.at(x + 1, y), x + 1, y)
        checkJob(player, Block.at(x - 1, y), x - 1, y)
        checkJob(player, Block.at(x, y + 1), x, y + 1)
        checkJob(player, Block.at(x, y - 1), x, y - 1)
    end
end

job {
    name = "convert_floor",
    run = function(unit, job)
        local success, cooldown = unit:getAbility("convert"):use()
        if success then
            return cooldown
        else
            local jx, jy = job:getPosition()
            return unit:move(jx, jy)
        end
    end,
    events = {
        onBlockTypeChanged = convertFloorBlockChangeHandler,
        onBlockOwnerChanged = convertFloorBlockChangeHandler
    }
}

--[[
-----------------------------------------------------------
Wall conversion.
-----------------------------------------------------------
]]--

--[[
When a block's meta or owner changes we want to destroy all jobs targeting it,
then try to recreate it (target validity changed).
We also want to update all neighboring blocks, because a slot might have become
open (location validity changed).
--]]
local function convertWallBlockChangeHandler(block)
    for player = 1, 4 do
        Job.deleteByTypeWhereTarget(player, "convert_wall", block)

        local function validateLocation(block)
            return block:isPassable() and block:getOwner() == player
        end
        local function validateTarget(block)
            -- TODO allow repairs
            return block:isConvertible() and
                    not block:isPassable() and
                    not block:isSelectedBy(player) and
                    block:getOwner() ~= player
        end
        util.addJobsForBlock(player, block, "convert_wall", validateLocation, validateTarget)
        util.addJobsForBlocksSurrounding(player, block, "convert_wall", validateLocation, validateTarget)
    end
end

job {
    name = "convert_wall",
    run = function(unit, job)
        local success, cooldown = unit:getAbility("convert"):use()
        if success then
            return cooldown
        else
            local jx, jy = job:getPosition()
            return unit:move(jx, jy)
        end
    end,
    events = {
        onBlockTypeChanged = convertWallBlockChangeHandler,
        onBlockOwnerChanged = convertWallBlockChangeHandler,
        onBlockSelectionChanged = function(player, block)
            Job.deleteByTypeWhereTarget(player, "convert_wall", block)

            local function validateLocation(block)
                return block:isPassable() and block:getOwner() == player
            end
            local function validateTarget(block)
                -- TODO allow repairs
                return block:isConvertible() and
                        not block:isPassable() and
                        not block:isSelectedBy(player) and
                        block:getOwner() ~= player
            end
            util.addJobsForBlock(player, block, "convert_wall", validateLocation, validateTarget)
        end
    }
}
