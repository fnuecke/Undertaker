#include "config.h"

#include <stdio.h>

int MP_resolution_x = 1600;
int MP_resolution_y = 1200;
int MP_field_of_view = 90;
bool MP_use_antialiasing = false;
bool MP_use_fog = true;
FILE* MP_log_target;
bool MP_d_ai_enabled = true;
bool MP_d_draw_test_texture = false;
bool MP_d_draw_paths = false;
bool MP_d_draw_jobs = false;
MP_DisplayMode MP_d_draw_deferred = MP_D_DEFERRED_FINAL;
bool MP_d_draw_picking_mode = false;
bool MP_d_draw_deferred_shader = true;
bool MP_d_draw_light_volumes = false;

void MP_load_config(void) {
    MP_log_target = fopen("game.log", "w");
}

void MP_save_config(void) {
    fclose(MP_log_target);
}