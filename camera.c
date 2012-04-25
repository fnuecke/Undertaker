#include <math.h>

#include "camera.h"
#include "config.h"
#include "map.h"
#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

// Current camera position.
static vec2 gCameraPosition = {
    {0, 0}
};

// Speed with which the camera moves.
static vec2 gCameraVelocity = {
    {0, 0}
};

// Current scroll direction.
static DK_CameraDirection gCameraDirection;

// Current camera zoom level.
static float gCameraZoom = 0;

// Current target zoom level we interpolate towards.
static float gCameraZoomTarget = 0;

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

const vec2* DK_GetCameraPosition(void) {
    return &gCameraPosition;
}

float DK_GetCameraZoom(void) {
    return gCameraZoom;
}

void DK_CameraStartScrolling(DK_CameraDirection direction) {
    gCameraDirection |= direction;
}

void DK_CameraStopScrolling(DK_CameraDirection direction) {
    gCameraDirection &= ~direction;
}

void DK_CameraZoomIn(void) {
    gCameraZoomTarget += DK_CAMERA_ZOOM_STEP;
    if (gCameraZoomTarget > 1.0f) {
        gCameraZoomTarget = 1.0f;
    }
}

void DK_CameraZoomOut(void) {
    gCameraZoomTarget -= DK_CAMERA_ZOOM_STEP;
    if (gCameraZoomTarget < 0) {
        gCameraZoomTarget = 0;
    }
}

void DK_UpdateCamera(void) {
    if (gCameraDirection & DK_CAMERA_DIRECTION_NORTH) {
        gCameraVelocity.v[1] = DK_CAMERA_SPEED;
    }
    if (gCameraDirection & DK_CAMERA_DIRECTION_SOUTH) {
        gCameraVelocity.v[1] = -DK_CAMERA_SPEED;
    }
    if (gCameraDirection & DK_CAMERA_DIRECTION_EAST) {
        gCameraVelocity.v[0] = DK_CAMERA_SPEED;
    }
    if (gCameraDirection & DK_CAMERA_DIRECTION_WEST) {
        gCameraVelocity.v[0] = -DK_CAMERA_SPEED;
    }

    gCameraPosition.v[0] += gCameraVelocity.v[0];
    gCameraPosition.v[1] += gCameraVelocity.v[1];

    if (gCameraPosition.v[0] < 0) {
        gCameraPosition.v[0] = 0;
    }
    if (gCameraPosition.v[0] > DK_GetMapSize() * DK_BLOCK_SIZE) {
        gCameraPosition.v[0] = DK_GetMapSize() * DK_BLOCK_SIZE;
    }

    if (gCameraPosition.v[1] < 0) {
        gCameraPosition.v[1] = 0;
    }
    if (gCameraPosition.v[1] > DK_GetMapSize() * DK_BLOCK_SIZE) {
        gCameraPosition.v[1] = DK_GetMapSize() * DK_BLOCK_SIZE;
    }

    gCameraVelocity.v[0] *= DK_CAMERA_FRICTION;
    gCameraVelocity.v[1] *= DK_CAMERA_FRICTION;

    if (fabs(gCameraVelocity.v[0]) < 0.0001f) {
        gCameraVelocity.v[0] = 0;
    }
    if (fabs(gCameraVelocity.v[1]) < 0.0001f) {
        gCameraVelocity.v[1] = 0;
    }

    gCameraZoom += (gCameraZoomTarget - gCameraZoom) / 2.0f;
}
