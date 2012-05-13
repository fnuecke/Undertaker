--[[
This job makes units convert unselected blocks to belong to their owner. If the
unit is not close enough to the job, it will move there.

The unit will need to have the 'convert' ability, which is used to determine
conversion strength and cooldown.
--]]

-- Get utility methods for creating jobs for block walls.
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
When a block's meta changes we want to destroy all jobs targeting it. We also
want to update all neighboring blocks, because a conversion slot might have
become open (if our passability changed).
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
		if not addJobsForBlocksSurrounding(player, block, x, y, "convert_wall", validateLocation, validateTarget) then
			-- Invalid location, clear all jobs targeting neighboring blocks.
			for job in Job.getByType(player, "convert_wall") do
				local jx, jy = job:getPosition()
				if jx > x and jx < x + 1 and jy > y and jy < y + 1 then
					Job.delete(player, job)
				end
			end
		end
	end
end

--[[
Check if we're in range for converting. If so, convert, else go there and wait
until we reach the job's location.
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
		-- TODO replace with ability strength
		job:getTargetBlock():convert(unit:getOwner(), 10)
		-- TODO replace with ability cooldown
		return 0.5
	else
		-- Not yet there, move and try again after getting there.
		return unit:move(jx, jy)
	end
end

export("onBlockSelectionChanged", onBlockSelectionChanged)
export("onBlockMetaChanged", onBlockChanged)
export("onBlockOwnerChanged", onBlockChanged)
export("run", run)