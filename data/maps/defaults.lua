--[[

This file contains all default types for the basic game logic. They can be
overridden by simply redefining them.

]]--

-- Passability types. These must be declared before using them.
passability "land"
passability "air"
passability "water"
passability "lava"

--[[
Block types. These must be declared before using them. To create circular life
cycles (via becomes), redeclare the first / last block after the last / first
one has been declared. For example:
block {name="c"}
block {name="b", becomes="c"}
block {name="a", becomes="b"}
block {name="c", becomes="a"}
This will lead to a cycle of a->b->c->a.
]]-- 
block {name="rock"}
block {name="open", level="normal", passability="land", strength=100}
block {name="dirt", durability=60, strength=200, becomes="open"}
block {name="gold", durability=120, gold=1000, becomes="open"}
block {name="gem", durability=4000000, gold=40000000, becomes="rock"}
block {name="water", level="lowered", passability="water"}
block {name="lava", level="lowered", passability="lava"}
