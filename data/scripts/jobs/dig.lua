--[[
This job makes units destroy selected blocks. If the unit is not close enough to
the job, it will move there.

The unit will need to have the 'dig' ability, which is used to determine
dig strength and cooldown.
--]]

-- Get utility methods.
import "ability_util"
import "wall_util"

--[[
When a block is selected, mark it for digging, else remove the job.
--]]
local function onBlockSelectionChanged(player, block, x, y)
	Job.deleteByTypeWhereTarget(player, "dig", block)

	local function validateLocation(block)
		return block:isPassable()
	end
	local function validateTarget(block)
		return block:isSelectedBy(player)
	end
	addJobsForBlockAt(player, block, x, y, "dig", validateLocation, validateTarget)
end

--[[
When a block's meta changes we want to destroy all jobs targeting it. We also
want to update all neighboring blocks, because a dig slot might have become
open (if our passability changed).
--]]
local function onBlockMetaChanged(block, x, y)
	for player = 1, 4 do
		Job.deleteByTypeWhereTarget(player, "dig", block)

		local function validateLocation(block)
			return block:isPassable()
		end
		local function validateTarget(block)
			return block:isSelectedBy(player)
		end
		addJobsForBlockAt(player, block, x, y, "dig", validateLocation, validateTarget)
		addJobsForBlocksSurrounding(player, block, x, y, "dig", validateLocation, validateTarget)
	end
end

--[[
When the owner of a block changes, delete jobs for the block for all non-owners.
--]]
local function onBlockOwnerChanged(block, x, y)
	for player = 1, 4 do
		if player ~= block:getOwner() then
			Job.deleteByTypeWhereTarget(player, "dig", block)
		end
	end
end

--[[
We're active, so do some digging.
--]]
local function run(unit, job)
	local jx, jy = job:getPosition()
	useAbilityOrMove(unit, "dig", jx, jy, {block=job:getTargetBlock()})
end

export("onBlockSelectionChanged", onBlockSelectionChanged)
export("onBlockMetaChanged", onBlockMetaChanged)
export("onBlockOwnerChanged", onBlockOwnerChanged)
export("run", run)