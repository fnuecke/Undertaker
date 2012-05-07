-- Get default definitions.
meta "defaults"

-- Map size in blocks, always squared.
size 16

-- Terrain definition.
local map = {}
terrain map

--[[
0 0 0 1 1 1 1 1 1 0 0 0 0 0 0
0 0 1 1 1 1 1 1 1 1 0 1 1 0 0
1 1 1 1 1 entry 1 1 1 1 0 0 0
0 1 1 1 1       1 1 1 1 1 1 0
0 0 1 1 1       1 1 1 1 1 1 1
0 0 1 1 1 1 1 1 1 1 1 1 1 1 0
0 1 1 1 1 1 2 1 1 1 1 1 1 0 0
1 1 1 1  player0  1 1 1 1 1 0
1 1 1 1           1 1 1 1 1 0
1 1 1 2           2 1 1 1 1 0
1 1 1 1           1 1 1 1 1 1
1 1 1 1           1 1 1 1 1 1
1 1 1 1 1 1 2 1 1 1 1 1 1 1 0
0 1 1 1 3 3 1 3 3 1 1 1 1 1 1
1 1 1 1 3 3 3 1 3 1 1 1 1 0 0
0 0 1 1 3 1 3 3 1 1 1 1 0 0 0
]]
