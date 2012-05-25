--[[
Helper methods for using abilities or moving to a position to use them.
--]]

--[[
Utility method for making a unit use its ability or move to a specified location
so that the unit can use the ability (if using it fails).
--]]
local function useAbilityOrMove(unit, abilityName, x, y, args)
	args.x = x
	args.y = y
	local success, cooldown = unit:useAbility(abilityName, args)
	if success then
		-- All green, wait until the ability can be used again.
		return cooldown
	else
		-- Not yet there, move and try again after getting there.
		return unit:move(x, y)
	end
end

export("useAbilityOrMove", useAbilityOrMove)