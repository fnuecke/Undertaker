--[[
AI describing logic for the 'dig' job.
]]

-- Sides of a block (for job openings)
local Side = {
    NONE = 0,
    SAME = 1,
    NORTH = 2,
    EAST = 4,
    SOUTH = 8,
    WEST = 16
}

-- Check for block related jobs of a specified type for a specified block
function getExistingJobs(player, block, x, y)
    local existingJobs = Side.NONE
	for job in getJobsOfType(player, "dig") do
        if job.block == block then
			if (job.position.y < y) then
				-- Top.
				existingJobs = bit32.bor(existingJobs, Side.NORTH)
			elseif (job.position.y > y + 1) then
				-- Bottom.
				existingJobs = bit32.bor(existingJobs, Side.SOUTH)
			elseif (job.position.x < x) then
				-- Left.
				existingJobs = bit32.bor(existingJobs, Side.WEST)
			elseif (job.position.x > x + 1) then
				-- Right.
				existingJobs = bit32.bor(existingJobs, Side.EAST)
			else
				-- Same block.
				existingJobs = bit32.bor(existingJobs, Side.SAME)
			end
		end
    end
    return existingJobs
end

-- We want to be notified if the selection state of a block changed.
function onBlockSelectionChanged(player, block, x, y)
	if isBlockSelected(player, x, y) then
		-- It's selected, start digging if a neighboring tile is passable.
		local existingJobs = getExistingJobsForBlock(player, block, x, y)
		if (not bit32.band(existingJobs, Side.NORTH) and
				isBlockPassable(getBlockAt(x, y - 1))) then
			addBlockJob(player, block, {x + 0.5, y - 0.25}, "dig")
		end
		if (not bit32.band(existingJobs & Side.SOUTH) &&
				isBlockPassable(getBlockAt(x, y + 1))) then
			-- Bottom is valid.
			addBlockJob(player, block, {x + 0.5, y + 1.25}, "dig")
		end
		if (not bit32.band(existingJobs & Side.EAST) &&
				isBlockPassable(getBlockAt(x + 1, y))) then
			-- Right is valid.
			addBlockJob(player, block, {x + 1.25, y + 0.5}, "dig")
		end
		if (not bit32.band(existingJobs & Side.WEST) &&
				isBlockPassable(getBlockAt(x - 1, y))) then
			-- Left is valid.
			addBlockJob(player, block, {x - 0.25, y + 0.5}, "dig")
		end
	end
end

function run()

end