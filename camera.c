#include <math.h>

#include "camera.h"
#include "config.h"
#include "map.h"
#include "vmath.h"

DK_CameraDirection camera_direction;
vec2 camera_velocity = {
    {0, 0}
};
float camera_friction = 0;
vec2 camera_position = {
    {0, 0}
};
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

const vec2* DK_camera_position(void) {
    return &camera_position;
}

float DK_camera_zoom(void) {
    return camera_zoom;
}

void DK_update_camera(void) {
    if (camera_direction & DK_CAMD_NORTH) {
        camera_velocity.v[1] = DK_CAMERA_SPEED;
    }
    if (camera_direction & DK_CAMD_SOUTH) {
        camera_velocity.v[1] = -DK_CAMERA_SPEED;
    }
    if (camera_direction & DK_CAMD_EAST) {
        camera_velocity.v[0] = DK_CAMERA_SPEED;
    }
    if (camera_direction & DK_CAMD_WEST) {
        camera_velocity.v[0] = -DK_CAMERA_SPEED;
    }

    camera_position.v[0] += camera_velocity.v[0];
    camera_position.v[1] += camera_velocity.v[1];

    if (camera_position.v[0] < 0) {
        camera_position.v[0] = 0;
    }
    if (camera_position.v[0] > DK_map_size * DK_BLOCK_SIZE) {
        camera_position.v[0] = DK_map_size * DK_BLOCK_SIZE;
    }

    if (camera_position.v[1] < 0) {
        camera_position.v[1] = 0;
    }
    if (camera_position.v[1] > DK_map_size * DK_BLOCK_SIZE) {
        camera_position.v[1] = DK_map_size * DK_BLOCK_SIZE;
    }

    camera_velocity.v[0] *= DK_CAMERA_FRICTION;
    camera_velocity.v[1] *= DK_CAMERA_FRICTION;

    if (fabs(camera_velocity.v[0]) < 0.0001f) {
        camera_velocity.v[0] = 0;
    }
    if (fabs(camera_velocity.v[1]) < 0.0001f) {
        camera_velocity.v[1] = 0;
    }

    camera_zoom += (camera_zoom_target - camera_zoom) / 2.0f;
}
