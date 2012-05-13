--[[
This job makes units destroy selected blocks. If the unit is not close enough to
the job, it will move there.

The unit will need to have the 'dig' ability, which is used to determine
dig strength and cooldown.
--]]

-- Get utility methods for creating jobs for block walls.
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
		if not addJobsForBlocksSurrounding(player, block, x, y, "dig", validateLocation, validateTarget) then
			-- Invalid location, clear all jobs targeting neighboring blocks.
			for job in Job.getByType(player, "dig") do
				local jx, jy = job:getPosition()
				if jx > x and jx < x + 1 and jy > y and jy < y + 1 then
					Job.delete(player, job)
				end
			end
		end
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
Check if we're in range for digging. If so, dig, else go there and wait until
we reach the job's location.
--]]
local function run(unit, job)
	-- See whether we're close enough.
	local ux, uy = unit:getPosition()
	local jx, jy = job:getPosition()
	local dx, dy = ux - jx, uy - jy
	local dn = (dx * dx + dy * dy)
	-- TODO replace with ability range^2
	if dn <= 0.01 then
		-- In range, do our thing.
		-- TODO replace with ability damage
		job:getTargetBlock():damage(10)
		-- TODO replace with ability cooldown
		return 0.5
	else
		-- Not yet there, move and try again after getting there.
		return unit:move(jx, jy)
	end
end

export("onBlockSelectionChanged", onBlockSelectionChanged)
export("onBlockMetaChanged", onBlockMetaChanged)
export("onBlockOwnerChanged", onBlockOwnerChanged)
export("run", run)