#include "config.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


FILE* MP_logTarget;

int MP_resolutionX = 1280;
int MP_resolutionY = 1024;
int MP_fieldOfView = 90;
bool MP_antialiasing = false;
float MP_scrollSpeed = 10;
char* MP_log_file = NULL;

bool MP_DBG_isAIEnabled = true;
bool MP_DBG_drawTestTexture = false;
bool MP_DBG_drawPaths = false;
bool MP_DBG_drawJobs = false;
MP_DBG_DisplayBuffer MP_DBG_deferredBuffer = MP_DBG_BUFFER_FINAL;
bool MP_DBG_drawPickingMode = false;
bool MP_DBG_useDeferredShader = true;
bool MP_DBG_drawLightVolumes = false;

void MP_LoadConfig(void) {
    lua_State* L = luaL_newstate();

    if (luaL_dofile(L, "data/config.lua") == LUA_OK) {
        lua_getglobal(L, "resolution");
        if (lua_istable(L, -1)) {
            unsigned int x, y;
            int isnum;
            lua_rawgeti(L, -1, 1);
            x = lua_tounsignedx(L, -1, &isnum);
            if (!isnum) {
                x = MP_resolutionX;
            }
            lua_pop(L, 1);
            lua_rawgeti(L, -1, 2);
            y = lua_tounsignedx(L, -1, &isnum);
            if (!isnum) {
                y = MP_resolutionY;
            }
            lua_pop(L, 1);

            if (x < 640) {
                x = 640;
            }
            if (x > 2560) {
                x = 2560;
            }
            if (y < 480) {
                y = 480;
            }
            if (y > 1600) {
                y = 1600;
            }

            MP_resolutionX = x;
            MP_resolutionY = y;
        }
        lua_pop(L, 1);

        lua_getglobal(L, "fov");
        if (lua_isnumber(L, -1)) {
            MP_fieldOfView = lua_tonumber(L, -1);
            if (MP_fieldOfView < 60) {
                MP_fieldOfView = 60;
            }
            if (MP_fieldOfView > 110) {
                MP_fieldOfView = 110;
            }
        }
        lua_pop(L, 1);

        lua_getglobal(L, "scrollspeed");
        if (lua_isnumber(L, -1)) {
            MP_scrollSpeed = lua_tonumber(L, -1);
            if (MP_scrollSpeed < 1) {
                MP_scrollSpeed = 1;
            }
            if (MP_scrollSpeed > 50) {
                MP_scrollSpeed = 50;
            }
        }
        lua_pop(L, 1);

        lua_getglobal(L, "logfile");
        if (lua_isstring(L, -1)) {
            const char* filename = lua_tostring(L, -1);
            if (!(MP_log_file = malloc((strlen(filename) + 1) * sizeof (char)))) {
                exit(EXIT_FAILURE);
            }
            strcpy(MP_log_file, filename);
        }
        lua_pop(L, 1);
    }

    lua_close(L);

    if (!MP_log_file || !(MP_logTarget = fopen(MP_log_file, "w"))) {
        const char* defaultname = "game.log";
        if (!(MP_log_file = malloc((strlen(defaultname) + 1) * sizeof (char)))) {
            exit(EXIT_FAILURE);
        }
        strcpy(MP_log_file, defaultname);
        MP_logTarget = fopen(MP_log_file, "w");
    }
}

void MP_SaveConfig(void) {
    FILE* f;

    MP_log_info("Writing settings.\n");

    if ((f = fopen("data/config.lua", "w"))) {
        fprintf(f, "-- This file is replaced each time the game exits!\n");
        fprintf(f, "resolution = {%d, %d}\n", MP_resolutionX, MP_resolutionY);
        fprintf(f, "fov = %d\n", MP_fieldOfView);
        fprintf(f, "scrollspeed = %.0f\n", MP_scrollSpeed);
        fprintf(f, "logfile = \"%s\"\n", MP_log_file);
        fclose(f);
    } else {
        MP_log_warning("Failed opening config file for writing.\n");
    }

    fclose(MP_logTarget);

    free(MP_log_file);
    MP_log_file = NULL;
}
