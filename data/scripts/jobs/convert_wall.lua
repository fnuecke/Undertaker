--[[
This job makes units convert unselected blocks to belong to their owner. If the
unit is not close enough to the job, it will move there.

The unit will need to have the 'convert' ability, which is used to determine
conversion strength and cooldown.
--]]

-- Get utility methods.
import "ability_util"
import "wall_util"

--[[
When a block is deselected, mark it for conversion, else remove the job.
--]]
local function onBlockSelectionChanged(player, block, x, y)
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
	addJobsForBlockAt(player, block, x, y, "convert_wall", validateLocation, validateTarget)
end

--[[
When a block's meta or owner changes we want to destroy all jobs targeting it,
then try to recreate it (target validity changed).
We also want to update all neighboring blocks, because a slot might have become
open (location validity changed).
--]]
local function onBlockChanged(block, x, y)
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
		addJobsForBlockAt(player, block, x, y, "convert_wall", validateLocation, validateTarget)
		addJobsForBlocksSurrounding(player, block, x, y, "convert_wall", validateLocation, validateTarget)
	end
end

--[[
We're active, so do some converting.
--]]
local function run(unit, job)
	local jx, jy = job:getPosition()
	useAbilityOrMove(unit, "convert", jx, jy, {block=job:getTargetBlock()})
end

export("onBlockSelectionChanged", onBlockSelectionChanged)
export("onBlockMetaChanged", onBlockChanged)
export("onBlockOwnerChanged", onBlockChanged)
export("run", run)