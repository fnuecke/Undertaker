--[[
This ability converts a block.
--]]

local function run(unit, config, args)
	-- See whether we're close enough.
	local ux, uy = unit:getPosition()
	local dx, dy = ux - args.x, uy - args.y
	local dn = (dx * dx + dy * dy)
	if dn <= 0.01 then
		-- In range, do our thing.
		args.block:convert(unit:getOwner(), config.strength)
		return true, config.cooldown
	else
		-- Not in range, report failure.
		return false
	end
end

export("run", run)