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

--[[
job "dig"
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
angerthreshold	= 0			satsifaction value below which a unit is angry
angerjob		= nil		the job a unit switches to when angry (unsatisfied)

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
preference		= 1			how strongly to weigh the job when on job search
initial			= 1			initial satisfaction value
unsatisfied		= 0			the threshold below which a unit is unsatisfied
satisfied		= 0			the threshold above which a unit is satisfied
bored			= 1			the threshold above which a unit is bored
performing		= 0			the delta per minute when performing the job
notperforming	= 0			the delta per minute when not performing the job

Not all these fields have to be set.
--]]
unit {name="imp", canpass={"land", "water"}, movespeed=1.8,
		jobs={
			{name="wander", preference=0}
		}}
--[[
Room types.

buildon "open"
room {name="heart", buildon="open", level="normal"		air			1300	0		gold
room {name="entrance", buildon="open", level="lowered		land		4000	0		spawn
room {name="treasure", buildon="open",		normal		land		100		50		gold
room {name="lair", buildon="open",		normal		land		200		100		sleep
room {name="hatchery", buildon="open",		lowered		land		350		125		eat
room {name="dojo", buildon="open",		normal		land		1250	150		train
room {name="library", buildon="open",		normal		land		320		200		research
room {name="workshop", buildon="open",		normal		land		900		200		craft
room {name="prison", buildon="open",		normal		land		600		225		rot
room {name="torture", buildon="open",		normal		land		1000	350		torture
room {name="graveyard", buildon="open",		lowered		land		350		300		revive
room {name="scavenge", buildon="open",		normal		land		1000	750		scavenge
room {name="temple", buildon="open",		normal		land		1000	350		pray
room {name="barrack", buildon="open",		normal		land		350		125		none
room {name="guardpost", buildon="open",		normal		land		5000	50		guard
room {name="bridge", buildon="water",		normal		land		100		30		none
room {name="stonebridge", buildon={"water","lava"},	normal		land		150		50		none
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