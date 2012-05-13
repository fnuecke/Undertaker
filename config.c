#include "config.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


FILE* MP_log_target;

int MP_resolution_x = 1280;
int MP_resolution_y = 1024;
int MP_field_of_view = 90;
bool MP_use_antialiasing = false;
float MP_scroll_speed = 10;
char* MP_log_file = NULL;

bool MP_d_ai_enabled = true;
bool MP_d_draw_test_texture = false;
bool MP_d_draw_paths = false;
bool MP_d_draw_jobs = false;
MP_DisplayMode MP_d_draw_deferred = MP_D_DEFERRED_FINAL;
bool MP_d_draw_picking_mode = false;
bool MP_d_draw_deferred_shader = true;
bool MP_d_draw_light_volumes = false;

void MP_load_config(void) {
    lua_State* L = luaL_newstate();

    if (luaL_dofile(L, "data/config.lua") == LUA_OK) {
        lua_getglobal(L, "resolution");
        if (lua_istable(L, -1)) {
            unsigned int x, y;
            int isnum;
            lua_rawgeti(L, -1, 1);
            x = lua_tounsignedx(L, -1, &isnum);
            if (!isnum) {
                x = MP_resolution_x;
            }
            lua_pop(L, 1);
            lua_rawgeti(L, -1, 2);
            y = lua_tounsignedx(L, -1, &isnum);
            if (!isnum) {
                y = MP_resolution_y;
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

            MP_resolution_x = x;
            MP_resolution_y = y;
        }
        lua_pop(L, 1);

        lua_getglobal(L, "fov");
        if (lua_isnumber(L, -1)) {
            MP_field_of_view = lua_tonumber(L, -1);
            if (MP_field_of_view < 60) {
                MP_field_of_view = 60;
            }
            if (MP_field_of_view > 110) {
                MP_field_of_view = 110;
            }
        }
        lua_pop(L, 1);

        lua_getglobal(L, "scrollspeed");
        if (lua_isnumber(L, -1)) {
            MP_scroll_speed = lua_tonumber(L, -1);
            if (MP_scroll_speed < 1) {
                MP_scroll_speed = 1;
            }
            if (MP_scroll_speed > 50) {
                MP_scroll_speed = 50;
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

    if (!MP_log_file || !(MP_log_target = fopen(MP_log_file, "w"))) {
        const char* defaultname = "game.log";
        if (!(MP_log_file = malloc((strlen(defaultname) + 1) * sizeof (char)))) {
            exit(EXIT_FAILURE);
        }
        strcpy(MP_log_file, defaultname);
        MP_log_target = fopen(MP_log_file, "w");
    }
}

void MP_save_config(void) {
    FILE* f;

    MP_log_info("Writing settings.\n");

    if ((f = fopen("data/config.lua", "w"))) {
        fprintf(f, "-- This file is replaced each time the game exits!\n");
        fprintf(f, "resolution = {%d, %d}\n", MP_resolution_x, MP_resolution_y);
        fprintf(f, "fov = %d\n", MP_field_of_view);
        fprintf(f, "scrollspeed = %.0f\n", MP_scroll_speed);
        fprintf(f, "logfile = \"%s\"\n", MP_log_file);
        fclose(f);
    } else {
        MP_log_warning("Failed opening config file for writing.\n");
    }

    fclose(MP_log_target);

    free(MP_log_file);
    MP_log_file = NULL;
}
