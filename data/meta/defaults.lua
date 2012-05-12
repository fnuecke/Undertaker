--[[

This file contains all default types for the basic game logic. They can be
overridden by simply redefining them.

In general, types have to be declared before they can be referenced. For
example, passability type must be declared before a block uses it, and a block
type must be declared before another uses it in the 'becomes' field, e.g.

--]]

--[[
Ability types.

TODO
--]]

--[[
Job types.

These represent single AI states. For each state declared here, there must be a
script file containing the actual logic for the state with the name of the job.
--]]
job "wander"
job "dig"

--[[
job "convert"

job "deliver"
job "explore"
job "eat"
job "sleep"
job "collect"
job "research"
job "train"
job "craft"
job "torture"
job "scavenge"
job "pray"
job "guard"
job "fight"
job "flee"
--]]

--[[
Passability types.

These are just names that can be used to reference a single passability type.
--]]
passability "land"
passability "air"
passability "water"
passability "lava"

--[[
Block types.

Use the 'block' directive to declare a block type.

To create circular life cycles (via becomes), redeclare the first / last block
after the last / first one has been declared. For example:
block {name="c"}
block {name="b", becomes="c"}
block {name="a", becomes="b"}
block {name="c", becomes="a"}
This will lead to a cycle of a->b->c->a.

Fields			Default			Info
name			= nil			obilgatory
level			= "normal"
passability		= nil			impassable
durability		= 0
strength		= 0
gold			= 0
becomes			= nil			obligatory if durability > 0

They can be changed using the 'blockdefaults' directive. All fields except the
name can be set to default to another value. Be careful to not set the 'becomes'
field to something that isn't declared, yet.
--]]
blockdefaults {level="high"}
block {name="rock"}
block {name="open", level="normal", passability="land", strength=100}
block {name="dirt", durability=60, strength=200, becomes="open"}
block {name="gold", durability=120, gold=1000, becomes="open"}
block {name="gem", durability=4000000, gold=40000000, becomes="rock"}
block {name="water", level="lowered", passability="water"}
block {name="lava", level="lowered", passability="lava"}

--[[
Unit types.

Each unit has a set of jobs which it pursues, as well as a few abilities, which
can be triggered by a job. A unit's actions are determined by its satisfaction
with each job.

A unit declaration has the following fields:
Field			Default		Info
name			= nil		obligatory
canpass			= nil		obligatory, passability types the unit can pass
movespeed		= 0			the base move speed of the unit
jobs			= nil		obligatory, see below
abilities		= nil		see below
onslap			= 0			how unit happiness changes when slapped (per minute)
inhand			= 0			how unit happiness changes when held (per minute)
angrybelow		= 0			satsifaction value below which a unit is angry
angerjob		= nil		the job a unit switches to when angry

The jobs field must be a table, containing in turn one table per job for the
unit. For each job a unit has a 'satisfaction' rating, which can vary in the
interval [0, 1]. There are a few thresholds which define how the unit behaves at
a specific section of that interval:
unsatisfied: the unit will not consider jobs it is not unsatisfied with when
looking for a job.
neutral: the job will be weighed with the unit's 'preference' for the job,
scaled from 1 to 0 in the section defined by 'unsatisfied' and 'satisfied'
threshold, respectively.
satisfied: the unit will be more likely to look for other jobs.
bored: the unit will not perform that job, even if it'll have to idle instead.

Note that for the unsatisfied threshold the value has to be less than the
threshold (satsifaction < threshold), and for the satisfied and bored thresholds
it has to be greater than the thresholds (satsisfaction > threshold). In
particular it's NOT less or equal / greater or equal (<= / >=). So if, e.g., the
bored threshold is set to 1 the unit will never get bored with the job.

The job preference is measured in blocks, i.e. if a unit is closer to job A,
but job B has a higher preference rating it might still pick B. The precise
calculation is:
	weight := (distance (in blocks) + preference) * (1 - weightedSatisfaction)
Where the weigted satisfaction is, as described above, the interval [0, 1]
mapped to the section from unsatisfied to satisfied thresholds for that job.

Simply put: if a unit should take job B even though job A is 10 blocks closer,
give job B a preference rating 10 larger than job A.

A single job's table contains the following fields:
Field			Default		Info
preference		= 0			how strongly to weigh the job when on job search
performing		= 0			the delta per minute when performing the job
notperforming	= 0			the delta per minute when not performing the job
initial			= 0			initial satisfaction value
unsatisfied		= 0			the threshold below which a unit is unsatisfied
satisfied		= 1			the threshold above which a unit is satisfied
bored			= 1			the threshold above which a unit is bored

Not all these fields have to be set.
--]]
unitdefaults {canpass={"land", "water"}}
unit {name="imp", movespeed=1.8,
		jobs={
			{name="wander", satisfied=0},
			{name="dig", preference=1}
		}}

--[[
Room types.

buildon "open"
roomdefaults {buildon="open", level="normal", passability="land"}
room {name="heart", passability="air", strength="1300"}
room {name="entrance", level="lowered", strength="4000", cost="0"}
room {name="treasure", strength="100", cost="50"}
room {name="lair", strength="200", cost="100"}
room {name="hatchery", level="lowered", strength="350", cost="125"}
room {name="dojo", strength="1250", cost="150"}
room {name="library", strength="320", cost="200"}
room {name="workshop", strength="900", cost="200"}
room {name="prison", strength="600", cost="225"}
room {name="torture", strength="1000", cost="350"}
room {name="graveyard", level="lowered", strength="350", cost="300"}
room {name="scavenge", strength="1000", cost="750"}
room {name="temple", strength="1000", cost="350"}
room {name="barrack", strength="350", cost="125"}
room {name="guardpost", strength="5000", cost="50"}
room {name="bridge", buildon="water", strength="100", cost="30"}
room {name="stonebridge", buildon={"water","lava"}, strength="150", cost="50"}
--]]

--[[
Door types.
--]]

--[[
doordefaults {buildon="open", visible=true}
door {name="wood", health=400, cost=500}
door {name="brace", health=750, cost=1000}
door {name="steel", health=1500, cost=1500}
door {name="magic", health=3000, cost=3000}
door {name="hidden", visible=false, health=1000, cost=6000}
--]]