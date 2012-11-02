import "util"

ability {
    name = "dig",
    run = function(ability)
        local unit = ability:getUnit()
        local properties = ability:getProperties()
        -- See whether we're close enough.
        if util.distanceToJob(unit) <= 0.01 then
            -- In range, do our thing.
            unit:getJob():getTarget():damage(properties.damage)
            return properties.cooldown
        else
            -- Not in range, report failure.
            return -1
        end
    end
}

job {
    name = "dig",
    run = function(unit, job)
        local result = unit:getAbility("dig"):use()
        if result >= 0 then
            return result, true
        else
            local jx, jy = job:getPosition()
            return unit:move(jx, jy)
        end
    end,
    events = {
        onBlockSelectionChanged = function(player, block)
            Job.deleteByTypeWhereTarget(player, "dig", block)

            local function validateLocation(block)
                return block:isPassable()
            end
            local function validateTarget(block)
                return block:isSelectedBy(player) 
            end
            local x, y = block:getPosition()
            util.addJobsForBlock(player, block, "dig", validateLocation, validateTarget)
        end,

        --[[
        When a block's meta changes we want to destroy all jobs targeting it. We also
        want to update all neighboring blocks, because a dig slot might have become
        open (if our passability changed).
        --]]
        onBlockTypeChanged = function(block)
            local x, y = block:getPosition()
            for player = 1, 4 do
                Job.deleteByTypeWhereTarget(player, "dig", block)

                local function validateLocation(block)
                    return block:isPassable()
                end
                local function validateTarget(block)
                    return block:isSelectedBy(player)
                end
                util.addJobsForBlock(player, block, "dig", validateLocation, validateTarget)
                util.addJobsForBlocksSurrounding(player, block, "dig", validateLocation, validateTarget)
            end
        end,

        --[[
        When the owner of a block changes, delete jobs for the block for all non-owners.
        --]]
        onBlockOwnerChanged = function(block)
            for player = 1, 4 do
                if player ~= block:getOwner() then
                    Job.deleteByTypeWhereTarget(player, "dig", block)
                end
            end
        end
    }
}
