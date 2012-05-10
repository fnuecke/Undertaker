-- Register the wander job for the newly added unit.
function onUnitAdded(unit)
	-- We target ourself.
	Job.create {name="wander", unit=unit}
end

function run(unit)
	-- This is the maximum distance how far we may wander.
	local WANDER_RANGE = 2
	-- This is the minimum delay in seconds before wandering again.
	local WANDER_DELAY_MIN = 2
	-- This is the variance for the delay, so that
	-- WANDER_DELAY_MAX = WANDER_DELAY_MIN + WANDER_DELAY_VARIANCE
	local WANDER_DELAY_VARIANCE = 2

    -- Just walk around dumbly.
	local x, y = unit:getPosition()
	x = x + WANDER_RANGE * (math.random() - 0.5)
	y = y + WANDER_RANGE * (math.random() - 0.5)

    -- TODO Make sure the unit doesn't wander too close to a wall.
    --[[
        if (position.d.x < 0.2f) {
            position.d.x = 0.2f;
        }
        if (position.d.x > DK_GetMapSize() - 0.2f) {
            position.d.x = DK_GetMapSize() - 0.2f;
        }
        if (position.d.y < 0.2f) {
            position.d.y = 0.2f;
        }
        if (position.d.y > DK_GetMapSize() - 0.2f) {
            position.d.y = DK_GetMapSize() - 0.2f;
        }
    ]]

    -- Check if the block is valid for the unit.
    if Block.at(math.floor(x), math.floor(y)):isPassableBy(unit) then
        -- OK go.
        unit:move(x, y)

        -- Update delay. Wait some before wandering again.
        state.delay = WANDER_DELAY_MIN + (math.random() * WANDER_DELAY_VARIANCE)
    end
end
