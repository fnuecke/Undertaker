#include <stdio.h>

#include "config.h"

int DK_resolution_x = 1280;
int DK_resolution_y = 1024;
int DK_field_of_view = 90;
int DK_use_antialiasing = 1;
int DK_use_fog = 1;
FILE* DK_log_target;
int DK_d_draw_test_texture = 0;
int DK_d_draw_paths = 0;
int DK_d_draw_jobs = 0;

void DK_load_config(void) {
    DK_log_target = fopen("game.log", "w");
}

void DK_save_config(void) {
    fclose(DK_log_target);
}