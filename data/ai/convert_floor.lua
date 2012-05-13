--[[
This job makes units convert passable blocks to belong to their owner. If the
unit is not close enough to the job, it will move there.

The unit will need to have the 'convert' ability, which is used to determine
conversion strength and cooldown.
--]]

--[[
Check if a block is a floor tile that's owned by the specified player.
--]]
local function isFloorOwnedBy(player, block)
	return block:isPassable() and block:getOwner() == player
end

--[[
Checks if we need a job at the specified block, and if so checks if there is one
and creates one if not. Otherwise deletes possibly existing jobs.
--]]
local function checkJob(player, block, x, y)
	-- TODO allow repairs
	if block:isConvertible() and block:isPassable() and block:getOwner() ~= player then
		if isFloorOwnedBy(player, Block.at(x - 1, y)) or
			isFloorOwnedBy(player, Block.at(x + 1, y)) or
			isFloorOwnedBy(player, Block.at(x, y - 1)) or
			isFloorOwnedBy(player, Block.at(x, y + 1))
		then
			if not block:isTargetOfJobByType(player, "convert_floor") then
				Job.create {name="convert_floor", player=player, block=block}
			end
			return
		end
	end
	-- Fail case -- not valid for being converted.
	Job.deleteByTypeWhereTarget(player, "convert_floor", block)
end

--[[
Used for both meta and owner changes. Checks self and neighboring tiles for
validity and creates / deletes jobs as necessary.
--]]
local function onBlockChanged(block, x, y)
	for player = 1, 4 do
		checkJob(player, block, x, y)
		checkJob(player, Block.at(x + 1, y), x + 1, y)
		checkJob(player, Block.at(x - 1, y), x - 1, y)
		checkJob(player, Block.at(x, y + 1), x, y + 1)
		checkJob(player, Block.at(x, y - 1), x, y - 1)
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

export("onBlockMetaChanged", onBlockChanged)
export("onBlockOwnerChanged", onBlockChanged)
export("run", run)