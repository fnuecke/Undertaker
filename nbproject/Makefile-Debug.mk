#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/script_loading_block.o \
	${OBJECTDIR}/lua/lstate.o \
	${OBJECTDIR}/script_lib_room.o \
	${OBJECTDIR}/picking.o \
	${OBJECTDIR}/bitset.o \
	${OBJECTDIR}/job.o \
	${OBJECTDIR}/shader.o \
	${OBJECTDIR}/graphics.o \
	${OBJECTDIR}/frustum.o \
	${OBJECTDIR}/simplexnoise.o \
	${OBJECTDIR}/script_events.o \
	${OBJECTDIR}/room_type.o \
	${OBJECTDIR}/lua/lcode.o \
	${OBJECTDIR}/room.o \
	${OBJECTDIR}/ability.o \
	${OBJECTDIR}/timer.o \
	${OBJECTDIR}/lua/lmathlib.o \
	${OBJECTDIR}/lua/lfunc.o \
	${OBJECTDIR}/lua/ldblib.o \
	${OBJECTDIR}/render.o \
	${OBJECTDIR}/lua/lstrlib.o \
	${OBJECTDIR}/lua/loadlib.o \
	${OBJECTDIR}/unit.o \
	${OBJECTDIR}/selection.o \
	${OBJECTDIR}/script_loading_ability.o \
	${OBJECTDIR}/lua/ldebug.o \
	${OBJECTDIR}/dbg_job.o \
	${OBJECTDIR}/script_lib_unit.o \
	${OBJECTDIR}/map.o \
	${OBJECTDIR}/lua/ltm.o \
	${OBJECTDIR}/unit_type.o \
	${OBJECTDIR}/lua/linit.o \
	${OBJECTDIR}/script_lib_job.o \
	${OBJECTDIR}/events.o \
	${OBJECTDIR}/job_type.o \
	${OBJECTDIR}/lua/ltablib.o \
	${OBJECTDIR}/block.o \
	${OBJECTDIR}/lua/lctype.o \
	${OBJECTDIR}/script_lib_block.o \
	${OBJECTDIR}/init.o \
	${OBJECTDIR}/lua/loslib.o \
	${OBJECTDIR}/lua/lzio.o \
	${OBJECTDIR}/lua/lopcodes.o \
	${OBJECTDIR}/lua/lgc.o \
	${OBJECTDIR}/script_lib.o \
	${OBJECTDIR}/lua/liolib.o \
	${OBJECTDIR}/lua/ldo.o \
	${OBJECTDIR}/lua/lobject.o \
	${OBJECTDIR}/lua/lbaselib.o \
	${OBJECTDIR}/script_lib_ability.o \
	${OBJECTDIR}/lua/lbitlib.o \
	${OBJECTDIR}/lua/lmem.o \
	${OBJECTDIR}/unit_ai.o \
	${OBJECTDIR}/block_type.o \
	${OBJECTDIR}/lua/lundump.o \
	${OBJECTDIR}/astar.o \
	${OBJECTDIR}/ability_type.o \
	${OBJECTDIR}/dbg_unit.o \
	${OBJECTDIR}/lua/ldump.o \
	${OBJECTDIR}/lua/lstring.o \
	${OBJECTDIR}/quadtree.o \
	${OBJECTDIR}/config.o \
	${OBJECTDIR}/lua/lvm.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/astar_mp.o \
	${OBJECTDIR}/lua/llex.o \
	${OBJECTDIR}/script_loading_job.o \
	${OBJECTDIR}/script.o \
	${OBJECTDIR}/cursor.o \
	${OBJECTDIR}/script_loading_unit.o \
	${OBJECTDIR}/lua/lcorolib.o \
	${OBJECTDIR}/lua/lauxlib.o \
	${OBJECTDIR}/lua/lparser.o \
	${OBJECTDIR}/hand.o \
	${OBJECTDIR}/vmath.o \
	${OBJECTDIR}/passability.o \
	${OBJECTDIR}/lua/lapi.o \
	${OBJECTDIR}/camera.o \
	${OBJECTDIR}/lua/ltable.o \
	${OBJECTDIR}/textures.o \
	${OBJECTDIR}/map_loader.o


# C Compiler Flags
CFLAGS=-pedantic -ansi -std=c99 -Wshadow -Wdeclaration-after-statement -Wall -Wextra -Wfloat-equal -Wwrite-strings -Winit-self -Wcast-align -Wcast-qual -Wpointer-arith -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wmissing-prototypes -Wno-cpp -D_GCC -DWIN32 -DGLEW_STATIC

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/C\MinGW\msys\1.0\local\lib -static -lmingw32 -lglew32 -lopengl32 -lglu32 -lSDL_image -lSDLmain -lSDL -lpng -lz -mwindows

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/undertaker

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/undertaker: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/undertaker ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/script_loading_block.o: script_loading_block.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_loading_block.o script_loading_block.c

${OBJECTDIR}/lua/lstate.o: lua/lstate.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lstate.o lua/lstate.c

${OBJECTDIR}/script_lib_room.o: script_lib_room.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_lib_room.o script_lib_room.c

${OBJECTDIR}/picking.o: picking.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/picking.o picking.c

${OBJECTDIR}/bitset.o: bitset.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/bitset.o bitset.c

${OBJECTDIR}/job.o: job.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/job.o job.c

${OBJECTDIR}/shader.o: shader.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/shader.o shader.c

${OBJECTDIR}/graphics.o: graphics.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/graphics.o graphics.c

${OBJECTDIR}/frustum.o: frustum.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/frustum.o frustum.c

${OBJECTDIR}/simplexnoise.o: simplexnoise.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/simplexnoise.o simplexnoise.c

${OBJECTDIR}/script_events.o: script_events.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_events.o script_events.c

${OBJECTDIR}/room_type.o: room_type.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/room_type.o room_type.c

${OBJECTDIR}/lua/lcode.o: lua/lcode.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lcode.o lua/lcode.c

${OBJECTDIR}/room.o: room.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/room.o room.c

${OBJECTDIR}/ability.o: ability.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/ability.o ability.c

${OBJECTDIR}/timer.o: timer.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/timer.o timer.c

${OBJECTDIR}/lua/lmathlib.o: lua/lmathlib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lmathlib.o lua/lmathlib.c

${OBJECTDIR}/lua/lfunc.o: lua/lfunc.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lfunc.o lua/lfunc.c

${OBJECTDIR}/lua/ldblib.o: lua/ldblib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/ldblib.o lua/ldblib.c

${OBJECTDIR}/render.o: render.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/render.o render.c

${OBJECTDIR}/lua/lstrlib.o: lua/lstrlib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lstrlib.o lua/lstrlib.c

${OBJECTDIR}/lua/loadlib.o: lua/loadlib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/loadlib.o lua/loadlib.c

${OBJECTDIR}/unit.o: unit.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/unit.o unit.c

${OBJECTDIR}/selection.o: selection.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/selection.o selection.c

${OBJECTDIR}/script_loading_ability.o: script_loading_ability.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_loading_ability.o script_loading_ability.c

${OBJECTDIR}/lua/ldebug.o: lua/ldebug.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/ldebug.o lua/ldebug.c

${OBJECTDIR}/dbg_job.o: dbg_job.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/dbg_job.o dbg_job.c

${OBJECTDIR}/script_lib_unit.o: script_lib_unit.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_lib_unit.o script_lib_unit.c

${OBJECTDIR}/map.o: map.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/map.o map.c

${OBJECTDIR}/lua/ltm.o: lua/ltm.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/ltm.o lua/ltm.c

${OBJECTDIR}/unit_type.o: unit_type.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/unit_type.o unit_type.c

${OBJECTDIR}/lua/linit.o: lua/linit.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/linit.o lua/linit.c

${OBJECTDIR}/script_lib_job.o: script_lib_job.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_lib_job.o script_lib_job.c

${OBJECTDIR}/events.o: events.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/events.o events.c

${OBJECTDIR}/job_type.o: job_type.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/job_type.o job_type.c

${OBJECTDIR}/lua/ltablib.o: lua/ltablib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/ltablib.o lua/ltablib.c

${OBJECTDIR}/block.o: block.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/block.o block.c

${OBJECTDIR}/lua/lctype.o: lua/lctype.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lctype.o lua/lctype.c

${OBJECTDIR}/script_lib_block.o: script_lib_block.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_lib_block.o script_lib_block.c

${OBJECTDIR}/init.o: init.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/init.o init.c

${OBJECTDIR}/lua/loslib.o: lua/loslib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/loslib.o lua/loslib.c

${OBJECTDIR}/lua/lzio.o: lua/lzio.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lzio.o lua/lzio.c

${OBJECTDIR}/lua/lopcodes.o: lua/lopcodes.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lopcodes.o lua/lopcodes.c

${OBJECTDIR}/lua/lgc.o: lua/lgc.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lgc.o lua/lgc.c

${OBJECTDIR}/script_lib.o: script_lib.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_lib.o script_lib.c

${OBJECTDIR}/lua/liolib.o: lua/liolib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/liolib.o lua/liolib.c

${OBJECTDIR}/lua/ldo.o: lua/ldo.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/ldo.o lua/ldo.c

${OBJECTDIR}/lua/lobject.o: lua/lobject.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lobject.o lua/lobject.c

${OBJECTDIR}/lua/lbaselib.o: lua/lbaselib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lbaselib.o lua/lbaselib.c

${OBJECTDIR}/script_lib_ability.o: script_lib_ability.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_lib_ability.o script_lib_ability.c

${OBJECTDIR}/lua/lbitlib.o: lua/lbitlib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lbitlib.o lua/lbitlib.c

${OBJECTDIR}/lua/lmem.o: lua/lmem.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lmem.o lua/lmem.c

${OBJECTDIR}/unit_ai.o: unit_ai.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/unit_ai.o unit_ai.c

${OBJECTDIR}/block_type.o: block_type.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/block_type.o block_type.c

${OBJECTDIR}/lua/lundump.o: lua/lundump.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lundump.o lua/lundump.c

${OBJECTDIR}/astar.o: astar.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/astar.o astar.c

${OBJECTDIR}/ability_type.o: ability_type.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/ability_type.o ability_type.c

${OBJECTDIR}/dbg_unit.o: dbg_unit.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/dbg_unit.o dbg_unit.c

${OBJECTDIR}/lua/ldump.o: lua/ldump.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/ldump.o lua/ldump.c

${OBJECTDIR}/lua/lstring.o: lua/lstring.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lstring.o lua/lstring.c

${OBJECTDIR}/quadtree.o: quadtree.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/quadtree.o quadtree.c

${OBJECTDIR}/config.o: config.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/config.o config.c

${OBJECTDIR}/lua/lvm.o: lua/lvm.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lvm.o lua/lvm.c

${OBJECTDIR}/main.o: main.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.c

${OBJECTDIR}/astar_mp.o: astar_mp.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/astar_mp.o astar_mp.c

${OBJECTDIR}/lua/llex.o: lua/llex.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/llex.o lua/llex.c

${OBJECTDIR}/script_loading_job.o: script_loading_job.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_loading_job.o script_loading_job.c

${OBJECTDIR}/script.o: script.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script.o script.c

${OBJECTDIR}/cursor.o: cursor.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/cursor.o cursor.c

${OBJECTDIR}/script_loading_unit.o: script_loading_unit.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_loading_unit.o script_loading_unit.c

${OBJECTDIR}/lua/lcorolib.o: lua/lcorolib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lcorolib.o lua/lcorolib.c

${OBJECTDIR}/lua/lauxlib.o: lua/lauxlib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lauxlib.o lua/lauxlib.c

${OBJECTDIR}/lua/lparser.o: lua/lparser.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lparser.o lua/lparser.c

${OBJECTDIR}/hand.o: hand.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/hand.o hand.c

${OBJECTDIR}/vmath.o: vmath.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/vmath.o vmath.c

${OBJECTDIR}/passability.o: passability.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/passability.o passability.c

${OBJECTDIR}/lua/lapi.o: lua/lapi.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lapi.o lua/lapi.c

${OBJECTDIR}/camera.o: camera.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/camera.o camera.c

${OBJECTDIR}/lua/ltable.o: lua/ltable.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/ltable.o lua/ltable.c

${OBJECTDIR}/textures.o: textures.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/textures.o textures.c

${OBJECTDIR}/map_loader.o: map_loader.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/map_loader.o map_loader.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/undertaker

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
