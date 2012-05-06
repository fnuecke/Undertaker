/**
 * Possible block types. The ID of the block type is determined by the order of
 * the entries in this file. The order may be controlled using the #id define:
 * 
 * blockdef // ID 0
 * #id 10
 * blockdef // ID 10
 * 
 * Blocks must define all of their properties. None may be omitted. If a
 * definition fails to parse, the ID will still be used up, using a default set
 * of attributes (unselectable and undestroyable).
 *
 * The first block (with ID 0) will be used as a default block when generating
 * empty maps (in editor mode).
 * 
 * Other files may be included using the #include command, e.g.
 * #include defaults-aux.h
 *
 * Properties are:
 */

// Passability types. 'none' is a reserved type for blocks/rooms that means it
// cannot be passed under any circumstances.
//	NAME
p	land
p	air
p	water
p	lava

/**
 * Blocks. These represent types (states) of a single block in a map.
 * Properties:
 * Name			String
 *		The name of the block, used for texture loading. The complete texture
 *		name consists of the the block name, level, side and ownership flag.
 *		For example, a 
 * Level		String
 *		The height at which to render this tile. Possible values are:
 *		pit			Infinite depth.
 *		lowered		Lower than normal.
 *		normal		Base height.
 *		high		Ceiling height.
 * Passability	String
 *		The passability types. A unit must have this ID registered in its
 *		passability list to be able to travel through this block. This must be
 *		a comma separated list without any blanks.
 * Durability	Integer
 *		How much "life" the block is, i.e. how long imps have to work on it
 *		before it crumbles. A block with zero durability is considered
 *		indestructible (and therefore unselectable).
 * Strength		Integer
 *		Base strength of the block, i.e. how long it takes to convert it or heal
 *		it. A block with zero strength is considered unconvertible.
 * Gold			Integer
 *		How much gold the block contains. The gold received by damaging the
 *		block is calculated as Gold/Durability*Damage, so that when it was
 *		completely destroyed the value set here will have been received.
 * Becomes		String
 *		The name of the block type this block will turn into upon destruction,
 *		i.e. when its durability reaches zero.
 */
//	NAME		LEVEL		PASSAB.		DUR.	STR.	GOLD	BECOMES
b	rock		high		none		0		0		0		rock
b	dirt		high		none		60		200		0		open
b	open		normal		land		0		100		0		open
b	gold		high		none		120		0		1000	open
b	gem			high		none		4e9		0		4e9		rock
b	water		lowered		water		0		0		0		water
b	lava		lowered		lava		0		0		0		lava
b	pit			pit			air			0		0		0		pit

// Rooms
//	NAME		BUILDON		LEVEL		PASSAB.		HEALTH	COST	JOB
r	heart		open		normal		air			1300	0		gold
r	entrance	open		lowered		land		4000	0		spawn
r	treasure	open		normal		land		100		50		gold
r	lair		open		normal		land		200		100		sleep
r	hatchery	open		lowered		land		350		125		eat
r	dojo		open		normal		land		1250	150		train
r	library		open		normal		land		320		200		research
r	workshop	open		normal		land		900		200		craft
r	prison		open		normal		land		600		225		rot
r	torture		open		normal		land		1000	350		torture
r	graveyard	open		lowered		land		350		300		revive
r	scavenge	open		normal		land		1000	750		scavenge
r	temple		open		normal		land		1000	350		pray
r	barrack		open		normal		land		350		125		none
r	guardpost	open		normal		land		5000	50		guard
r	bridge		water		normal		land		100		30		none
r	stonebridge	water,lava	normal		land		150		50		none

// Jobs. These are simply names for states of a units AI. The default job that
// always exists is the 'idle' job. In idle state units will look for the
// closest "proper" job. Rooms may be bound to rooms, i.e. they can only be
// pursued in certain rooms.
// The select field determines the actual target, where always the closest
// matching target will be chosen (of that type). The actually chosen job
// depends on the priorities for the unit with the job (see units).
// The action will determine what to do with the selected target.
//	NAME		SELECT				ACTION		PRIORITY			PRIO.TYPE
j	dig			block:selected		dig			target:distance		inverse
j	convert		block:unselected	convert		target:distance		inverse
j	explore		block:fogged		move		target:distance		inverse
j	wander		random:nearby		move		-					-
j	eat			room:hatchery		eat			hunger				linear
j	sleep		room:lair			sleep		fatigue				linear
j	research	room:library		research	distance
j	train		room:dojo			train		distance
j	craft		room:workshop		craft		distance
j	torture		room:torture		torture		distance
j	scavenge	room:scavenge		scavenge	distance
j	pray		room:temple			pray		distance
j	guard		room:guardpost		guard		distance
j	fight		unit:enemy			attack		distance
j	flee		room:heart			run			health				inverse

// Units. Each unit can follow a potential list of jobs, where each job has a
// "weight". If a job has a weight of 0 it is a last resort (any other job is
// better, regardless how far away).
//	NAME	JOBS						PASSAB.			MOVESPEED
u	imp		dig:10,convert,wander:0		land,water		1.8

// Doors
//	NAME		BUILDON		VISIBLE		HEALTH	COST
d	wood		open		true		400		500
d	brace		open		true		750		1000
d	steel		open		true		1500	1500
d	magic		open		true		3000	3000
d	hidden		open		false		1000	6000

// Towers
// ...

// Other game settings.

// Sell cost factor (gold returned = build cost * this factor).
v	sellFactor	0.5
