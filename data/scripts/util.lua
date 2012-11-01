--[[
Computes the distance of a unit to its job.
]]--
function util.distanceToJob(unit)
    local job = unit:getJob()
    local ux, uy = unit:getPosition()
    local jx, jy = job:getPosition()
    local dx, dy = ux - jx, uy - jy
    return math.sqrt(dx * dx + dy * dy)
end

--[[
Checks if a block is a floor block (passable) and owned by the specified player.
]]--
function util.isFloorOwnedBy(block, player)
    return block:isPassable() and block:getOwner() == player
end

--[[
Utility method for finding jobs for a specified block, on its neighboring
blocks. Uses validator methods to test block for job validity and neighbors
for job location validity.
--]]
function util.addJobsForBlock(player, block, jobType, validateLocation, validateTarget)
    local x, y = block:getPosition()
	-- Start digging if a neighboring tile is passable.
	if validateTarget(block) then
		local job = {name=jobType, player=player, block=block}
		if validateLocation(Block.at(x, y + 1)) then
			job.offset = {0, 0.7}
			Job.create(job)
		end
		if validateLocation(Block.at(x, y - 1)) then
			job.offset = {0, -0.7}
			Job.create(job)
		end
		if validateLocation(Block.at(x + 1, y)) then
			job.offset = {0.7, 0}
			Job.create(job)
		end
		if validateLocation(Block.at(x - 1, y)) then
			job.offset = {-0.7, 0}
			Job.create(job)
		end
	end
end

--[[
Utility method for finding jobs on a specified block, for its neighboring
blocks. Uses validator methods to test block for job validity and neighbors
for job location validity.
--]]
function util.addJobsForBlocksSurrounding(player, block, jobType, validateLocation, validateTarget)
    local x, y = block:getPosition()
	-- Can something reach jobs if they are on this block?
	if validateLocation(block) then
		-- Sides of a block (for job openings)
		local Side = {
			NONE = 0,
			NORTH = 1,
			EAST = 2,
			SOUTH = 4,
			WEST = 8
		}

		-- Check for dig jobs on neighboring blocks.
		local existingJobs = Side.NONE
		for job in Job.getByType(player, jobType) do
			local jx, jy = job:getPosition()
			if jx > x and jx < x + 1 and jy > y and jy < y + 1 then
				-- On this block, check where.
				if jy > y + 0.6 then
					existingJobs = bit32.bor(existingJobs, Side.NORTH)
				elseif jy < y + 0.4 then
					existingJobs = bit32.bor(existingJobs, Side.SOUTH)
				elseif jx > x + 0.6 then
					existingJobs = bit32.bor(existingJobs, Side.EAST)
				elseif jx < x + 0.4 then
					existingJobs = bit32.bor(existingJobs, Side.WEST)
				end
			end
		end

		-- Skip slots already occupied, check others using validator.
		local job = {name=jobType, player=player}
		if bit32.band(existingJobs, Side.NORTH) == 0 then
			local b = Block.at(x, y + 1)
			if validateTarget(b) then
				job.block = b
				job.offset = {0, -0.7}
				Job.create(job)
			end
		end
		if bit32.band(existingJobs, Side.SOUTH) == 0 then
			local b = Block.at(x, y - 1)
			if validateTarget(b) then
				job.block = b
				job.offset = {0, 0.7}
				Job.create(job)
			end
		end
		if bit32.band(existingJobs, Side.EAST) == 0 then
			local b = Block.at(x + 1, y)
			if validateTarget(b) then
				job.block = b
				job.offset = {-0.7, 0}
				Job.create(job)
			end
		end
		if bit32.band(existingJobs, Side.WEST) == 0 then
			local b = Block.at(x - 1, y)
			if validateTarget(b) then
				job.block = b
				job.offset = {0.7, 0}
				Job.create(job)
			end
		end
	else
		-- Invalid location, clear all jobs targeting neighboring blocks.
		for job in Job.getByType(player, jobType) do
			local jx, jy = job:getPosition()
			if jx > x and jx < x + 1 and jy > y and jy < y + 1 then
				Job.delete(player, job)
			end
		end
	end
end