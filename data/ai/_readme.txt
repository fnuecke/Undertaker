A job's AI is separated into two parts:
- Job generation
- Job execution

Jobs can generally categorized by their 'targets', i.e. the object of interest
for a job. The following target types are considered:
- Blocks
- Rooms
- Units
- None

Jobs dealing with blocks will normally generate job openings in the generation
phase, next to the block in question. Jobs dealing with rooms will also do so,
with the position somewhere in the room in question.

Job generation may be executed in predefined 'event handlers'. The engine will
look for a list of function names in an AI script, and, if present, call them
upon their related events. The methods follow the naming convention of an added
'on', followed by the camelcase event name. So, for example the event 'unit
A' will trigger a call to 'onUnitAdded'. The following events are available:

"block selection changed"
	called when the selection state of a single block on the current map
	changed.
	
	The event's signature is
		onBlockSelectionChanged(block, x, y, player)
	where
		block		is the block for which the selection state changed.
		x, y		are the map coordinates of the block.
		player		is the id of the player for whom the selection changed.

"block destroyed"
	called when a block is destroyed and has changed to its follow-up type (i.e.
	whatever is declared in the 'becomes' field in the blocks declaration).
	This is called after the block's type has been set to the new one.
	
	The event's signature is
		onBlockDestroyed(block, x, y)
	where
		block		is the block that was destroyed.
		x, y		are the map coordinates of the block.

"block converted"
	called when the ownership of a block changed.
	
	The event's signature is
		onBlockConverted(block, x, y, player)
	where
		block		is the block that has been converted.
		x, y		are the map coordinates of the block.
		player		is the id of the player that is the new owner of the block.

"room converted"
	called when a new room has come into the possession of a player. This can be
	either due to building one or conquering one.
	
	The event's signature is
		onRoomAdded(room, x, y, player)
	where
		room		is the room that was added.
		x, y		are the map coordinates of the room (block the room is on).
		player		the new owner of the room.

"unit added"
	called when a new unit was spawned on the map. This is also called on the
	AI of the unit that has been added, so it can be used to set-up some always
	valid jobs.
	
	The event's signature is
		onUnitAdded(unit, x, y, player)
	where
		unit		is the spawned unit.
		x, y		are the map coordiantes of the unit.
		player		is the owner of the unit.

Job execution refers to the part that is called each iteration of the main game
loop while the job is active. It is implemented by a method called 'run'. Note
that an AI script should never rely on globals, i.e. it must be state-less.

The method's signature is
	run(unit)
where
	unit 		is the unit for which to run the AI.
