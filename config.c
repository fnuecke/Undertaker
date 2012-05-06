#include <stdio.h>

#include "config.h"

int DK_resolution_x = 1600;
int DK_resolution_y = 1200;
int DK_field_of_view = 90;
bool DK_use_antialiasing = false;
bool DK_use_fog = true;
FILE* DK_log_target;
bool DK_d_ai_enabled = true;
bool DK_d_draw_test_texture = false;
bool DK_d_draw_paths = false;
bool DK_d_draw_jobs = false;
DK_DisplayMode DK_d_draw_deferred = DK_D_DEFERRED_FINAL;
bool DK_d_draw_picking_mode = false;
bool DK_d_draw_deferred_shader = true;
bool DK_d_draw_light_volumes = false;

void DK_load_config(void) {
    DK_log_target = fopen("game.log", "w");
}

void DK_save_config(void) {
    fclose(DK_log_target);
}