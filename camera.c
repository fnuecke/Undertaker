#include <math.h>

#include "camera.h"
#include "config.h"
#include "map.h"

DK_CameraDirection camera_direction;
float camera_velocity[] = {0, 0};
float camera_friction = 0;
float camera_position[] = {0, 0};
float camera_zoom = 0;
float camera_zoom_target = 0;

void DK_camera_set_direction(DK_CameraDirection direction) {
    camera_direction |= direction;
}

void DK_camera_unset_direction(DK_CameraDirection direction) {
    camera_direction &= ~direction;
}

void DK_camera_zoom_in(void) {
    camera_zoom_target += DK_CAMERA_ZOOM_STEP;
    if (camera_zoom_target > 1.0f) {
        camera_zoom_target = 1.0f;
    }
}

void DK_camera_zoom_out(void) {
    camera_zoom_target -= DK_CAMERA_ZOOM_STEP;
    if (camera_zoom_target < 0) {
        camera_zoom_target = 0;
    }
}

const float* DK_camera_position(void) {
    return camera_position;
}

float DK_camera_zoom(void) {
    return camera_zoom;
}

void DK_update_camera(void) {
    if (camera_direction & DK_CAMD_NORTH) {
        camera_velocity[1] = DK_CAMERA_SPEED;
    }
    if (camera_direction & DK_CAMD_SOUTH) {
        camera_velocity[1] = -DK_CAMERA_SPEED;
    }
    if (camera_direction & DK_CAMD_EAST) {
        camera_velocity[0] = DK_CAMERA_SPEED;
    }
    if (camera_direction & DK_CAMD_WEST) {
        camera_velocity[0] = -DK_CAMERA_SPEED;
    }

    camera_position[0] += camera_velocity[0];
    camera_position[1] += camera_velocity[1];

    if (camera_position[0] < 0) {
        camera_position[0] = 0;
    }
    if (camera_position[0] > DK_map_size * DK_BLOCK_SIZE) {
        camera_position[0] = DK_map_size * DK_BLOCK_SIZE;
    }

    if (camera_position[1] < 0) {
        camera_position[1] = 0;
    }
    if (camera_position[1] > DK_map_size * DK_BLOCK_SIZE) {
        camera_position[1] = DK_map_size * DK_BLOCK_SIZE;
    }

    camera_velocity[0] *= DK_CAMERA_FRICTION;
    camera_velocity[1] *= DK_CAMERA_FRICTION;

    if (fabs(camera_velocity[0]) < 0.0001f) {
        camera_velocity[0] = 0;
    }
    if (fabs(camera_velocity[1]) < 0.0001f) {
        camera_velocity[1] = 0;
    }

    camera_zoom += (camera_zoom_target - camera_zoom) / 2.0f;
}
