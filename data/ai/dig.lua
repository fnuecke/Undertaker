--[[
AI describing logic for the 'dig' job.
--]]

--[[
When a block is selected, mark it for digging, else unmark it.
--]]
function onBlockSelectionChanged(player, block, x, y)
	-- Delete all dig jobs for that block.
	Job.deleteByTypeWhereTarget(player, "dig", block)

	-- Check if the block is selected, if so create the dig jobs.
	if block:isSelectedBy(player) then
		-- Start digging if a neighboring tile is passable.
		local job = {name="dig", player=player, block=block}
		if Block.at(x, y + 1):isPassable() then
			job.offset = {0, 0.7}
			Job.create(job)
		end
		if Block.at(x, y - 1):isPassable() then
			job.offset = {0, -0.7}
			Job.create(job)
		end
		if Block.at(x + 1, y):isPassable() then
			job.offset = {0.7, 0}
			Job.create(job)
		end
		if Block.at(x - 1, y):isPassable() then
			job.offset = {-0.7, 0}
			Job.create(job)
		end
	end
end

--[[
When a block's meta changes we want to destroy all jobs targeting it. We also
want to update all neighboring blocks, because a dig slot might have become
open (if our passability changed).
--]]
function onBlockMetaChanged(block, x, y)
	for player = 1, 4 do
		-- Delete jobs targeting this block.
		Job.deleteByTypeWhereTarget(player, "dig", block)

		-- Can something reach jobs if they are on this block?
		if block:isPassable() then
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
			for job in Job.getByType(player, "dig") do
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

			-- Start digging if a neighboring tile if they're not already target
			-- of a dig job originating on this block, and they are selected.
			local job = {name="dig", player=player}
			if bit32.band(existingJobs, Side.NORTH) == 0 then
				local b = Block.at(x, y + 1)
				if b:isSelectedBy(player) then
					job.block = b
					job.offset = {0, -0.7}
					Job.create(job)
				end
			end
			if bit32.band(existingJobs, Side.SOUTH) == 0 then
				local b = Block.at(x, y - 1)
				if b:isSelectedBy(player) then
					job.block = b
					job.offset = {0, 0.7}
					Job.create(job)
				end
			end
			if bit32.band(existingJobs, Side.EAST) == 0 then
				local b = Block.at(x + 1, y)
				if b:isSelectedBy(player) then
					job.block = b
					job.offset = {-0.7, 0}
					Job.create(job)
				end
			end
			if bit32.band(existingJobs, Side.WEST) == 0 then
				local b = Block.at(x - 1, y)
				if b:isSelectedBy(player) then
					job.block = b
					job.offset = {0.7, 0}
					Job.create(job)
				end
			end
		else
			-- No unpassable, clear all jobs targeting neighboring blocks.
			for job in Job.getByType(player, "dig") do
				local jx, jy = job:getPosition()
				if jx > x and jx < x + 1 and jy > y and jy < y + 1 then
					-- On this block, check where.
					Job.delete(player, job)
				end
			end
		end
	end
end

--[[
When the owner of a block changes, delete jobs for the block for all non-owners.
--]]
function onBlockOwnerChanged(block, x, y)
	for player = 1, 4 do
		if player ~= block:getOwner() then
			Job.deleteByTypeWhereTarget(player, "dig", block)
		end
	end
end

function run(unit, job)
	-- See whether we're close enough.
	
end