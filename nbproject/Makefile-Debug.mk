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
CC=gcc.exe
CCC=g++.exe
CXX=g++.exe
FC=gfortran
AS=as.exe

# Macros
CND_PLATFORM=MinGW-Windows
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/51285538/job.o \
	${OBJECTDIR}/graphics.o \
	${OBJECTDIR}/script_loading_block.o \
	${OBJECTDIR}/lua/lstate.o \
	${OBJECTDIR}/script_lib_room.o \
	${OBJECTDIR}/_ext/51285538/selection.o \
	${OBJECTDIR}/script_events.o \
	${OBJECTDIR}/room_type.o \
	${OBJECTDIR}/lua/lcode.o \
	${OBJECTDIR}/room.o \
	${OBJECTDIR}/ability.o \
	${OBJECTDIR}/timer.o \
	${OBJECTDIR}/lua/lmathlib.o \
	${OBJECTDIR}/lua/lfunc.o \
	${OBJECTDIR}/_ext/51285538/render.o \
	${OBJECTDIR}/lua/ldblib.o \
	${OBJECTDIR}/_ext/51285538/unit.o \
	${OBJECTDIR}/_ext/51285538/cursor.o \
	${OBJECTDIR}/lua/lstrlib.o \
	${OBJECTDIR}/lua/loadlib.o \
	${OBJECTDIR}/_ext/51285538/astar.o \
	${OBJECTDIR}/script_loading_ability.o \
	${OBJECTDIR}/lua/ldebug.o \
	${OBJECTDIR}/dbg_job.o \
	${OBJECTDIR}/_ext/51285538/textures.o \
	${OBJECTDIR}/script_lib_unit.o \
	${OBJECTDIR}/lua/ltm.o \
	${OBJECTDIR}/unit_type.o \
	${OBJECTDIR}/lua/linit.o \
	${OBJECTDIR}/script_lib_job.o \
	${OBJECTDIR}/_ext/51285538/picking.o \
	${OBJECTDIR}/job_type.o \
	${OBJECTDIR}/lua/ltablib.o \
	${OBJECTDIR}/block.o \
	${OBJECTDIR}/lua/lctype.o \
	${OBJECTDIR}/script_lib_block.o \
	${OBJECTDIR}/_ext/51285538/init.o \
	${OBJECTDIR}/lua/loslib.o \
	${OBJECTDIR}/_ext/51285538/vmath.o \
	${OBJECTDIR}/_ext/51285538/config.o \
	${OBJECTDIR}/lua/lzio.o \
	${OBJECTDIR}/lua/lopcodes.o \
	${OBJECTDIR}/lua/lgc.o \
	${OBJECTDIR}/_ext/51285538/simplexnoise.o \
	${OBJECTDIR}/script_lib.o \
	${OBJECTDIR}/lua/liolib.o \
	${OBJECTDIR}/lua/ldo.o \
	${OBJECTDIR}/lua/lobject.o \
	${OBJECTDIR}/lua/lbaselib.o \
	${OBJECTDIR}/script_lib_ability.o \
	${OBJECTDIR}/lua/lbitlib.o \
	${OBJECTDIR}/_ext/51285538/bitset.o \
	${OBJECTDIR}/lua/lmem.o \
	${OBJECTDIR}/unit_ai.o \
	${OBJECTDIR}/_ext/51285538/shader.o \
	${OBJECTDIR}/lua/lundump.o \
	${OBJECTDIR}/block_type.o \
	${OBJECTDIR}/ability_type.o \
	${OBJECTDIR}/_ext/51285538/events.o \
	${OBJECTDIR}/dbg_unit.o \
	${OBJECTDIR}/_ext/51285538/map.o \
	${OBJECTDIR}/lua/lstring.o \
	${OBJECTDIR}/quadtree.o \
	${OBJECTDIR}/lua/ldump.o \
	${OBJECTDIR}/lua/lvm.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/astar_mp.o \
	${OBJECTDIR}/lua/llex.o \
	${OBJECTDIR}/script_loading_job.o \
	${OBJECTDIR}/script.o \
	${OBJECTDIR}/script_loading_unit.o \
	${OBJECTDIR}/lua/lcorolib.o \
	${OBJECTDIR}/lua/lauxlib.o \
	${OBJECTDIR}/lua/lparser.o \
	${OBJECTDIR}/_ext/51285538/camera.o \
	${OBJECTDIR}/hand.o \
	${OBJECTDIR}/passability.o \
	${OBJECTDIR}/lua/lapi.o \
	${OBJECTDIR}/lua/ltable.o \
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
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/undertaker.exe

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/undertaker.exe: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/undertaker ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/_ext/51285538/job.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/job.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/job.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/job.c

${OBJECTDIR}/graphics.o: graphics.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/graphics.o graphics.c

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

${OBJECTDIR}/_ext/51285538/selection.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/selection.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/selection.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/selection.c

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

${OBJECTDIR}/_ext/51285538/render.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/render.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/render.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/render.c

${OBJECTDIR}/lua/ldblib.o: lua/ldblib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/ldblib.o lua/ldblib.c

${OBJECTDIR}/_ext/51285538/unit.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/unit.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/unit.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/unit.c

${OBJECTDIR}/_ext/51285538/cursor.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/cursor.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/cursor.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/cursor.c

${OBJECTDIR}/lua/lstrlib.o: lua/lstrlib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lstrlib.o lua/lstrlib.c

${OBJECTDIR}/lua/loadlib.o: lua/loadlib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/loadlib.o lua/loadlib.c

${OBJECTDIR}/_ext/51285538/astar.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/astar.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/astar.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/astar.c

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

${OBJECTDIR}/_ext/51285538/textures.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/textures.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/textures.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/textures.c

${OBJECTDIR}/script_lib_unit.o: script_lib_unit.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/script_lib_unit.o script_lib_unit.c

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

${OBJECTDIR}/_ext/51285538/picking.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/picking.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/picking.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/picking.c

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

${OBJECTDIR}/_ext/51285538/init.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/init.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/init.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/init.c

${OBJECTDIR}/lua/loslib.o: lua/loslib.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/loslib.o lua/loslib.c

${OBJECTDIR}/_ext/51285538/vmath.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/vmath.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/vmath.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/vmath.c

${OBJECTDIR}/_ext/51285538/config.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/config.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/config.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/config.c

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

${OBJECTDIR}/_ext/51285538/simplexnoise.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/simplexnoise.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/simplexnoise.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/simplexnoise.c

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

${OBJECTDIR}/_ext/51285538/bitset.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/bitset.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/bitset.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/bitset.c

${OBJECTDIR}/lua/lmem.o: lua/lmem.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lmem.o lua/lmem.c

${OBJECTDIR}/unit_ai.o: unit_ai.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/unit_ai.o unit_ai.c

${OBJECTDIR}/_ext/51285538/shader.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/shader.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/shader.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/shader.c

${OBJECTDIR}/lua/lundump.o: lua/lundump.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lundump.o lua/lundump.c

${OBJECTDIR}/block_type.o: block_type.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/block_type.o block_type.c

${OBJECTDIR}/ability_type.o: ability_type.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/ability_type.o ability_type.c

${OBJECTDIR}/_ext/51285538/events.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/events.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/events.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/events.c

${OBJECTDIR}/dbg_unit.o: dbg_unit.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/dbg_unit.o dbg_unit.c

${OBJECTDIR}/_ext/51285538/map.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/map.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/map.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/map.c

${OBJECTDIR}/lua/lstring.o: lua/lstring.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lstring.o lua/lstring.c

${OBJECTDIR}/quadtree.o: quadtree.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/quadtree.o quadtree.c

${OBJECTDIR}/lua/ldump.o: lua/ldump.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/ldump.o lua/ldump.c

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

${OBJECTDIR}/_ext/51285538/camera.o: /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/camera.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/51285538
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/51285538/camera.o /C/Users/fnuecke/Documents/NetBeansProjects/Undertaker/camera.c

${OBJECTDIR}/hand.o: hand.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/hand.o hand.c

${OBJECTDIR}/passability.o: passability.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/passability.o passability.c

${OBJECTDIR}/lua/lapi.o: lua/lapi.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/lapi.o lua/lapi.c

${OBJECTDIR}/lua/ltable.o: lua/ltable.c 
	${MKDIR} -p ${OBJECTDIR}/lua
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/lua/ltable.o lua/ltable.c

${OBJECTDIR}/map_loader.o: map_loader.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -Wall -I/C\MinGW\msys\1.0\local\include -MMD -MP -MF $@.d -o ${OBJECTDIR}/map_loader.o map_loader.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/undertaker.exe

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
