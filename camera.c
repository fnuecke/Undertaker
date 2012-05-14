#include "camera.h"

#include <math.h>

#include "config.h"
#include "events.h"
#include "map.h"
#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

// Current camera position.
static vec3 gCameraPosition = {
    {0, 0, 0}
};

// Speed with which the camera moves.
static vec2 gCameraVelocity = {
    {0, 0}
};

// Current camera target.
static vec3 gCameraTarget = {
    {0, 0, 0}
};

// Current scroll direction.
static MP_CameraDirection gCameraDirection;

// Current camera zoom level.
static float gCameraZoom = 0;

// Current target zoom level we interpolate towards.
static float gCameraZoomTarget = 0;

///////////////////////////////////////////////////////////////////////////////
// Updating
///////////////////////////////////////////////////////////////////////////////

static void update(void) {
    if (gCameraDirection & MP_CAMERA_DIRECTION_NORTH) {
        gCameraVelocity.v[1] = MP_scrollSpeed;
    }
    if (gCameraDirection & MP_CAMERA_DIRECTION_SOUTH) {
        gCameraVelocity.v[1] = -MP_scrollSpeed;
    }
    if (gCameraDirection & MP_CAMERA_DIRECTION_EAST) {
        gCameraVelocity.v[0] = MP_scrollSpeed;
    }
    if (gCameraDirection & MP_CAMERA_DIRECTION_WEST) {
        gCameraVelocity.v[0] = -MP_scrollSpeed;
    }

    gCameraPosition.v[0] += gCameraVelocity.v[0] * MP_BLOCK_SIZE / MP_FRAMERATE;
    gCameraPosition.v[1] += gCameraVelocity.v[1] * MP_BLOCK_SIZE / MP_FRAMERATE;

    if (gCameraPosition.v[0] < 0) {
        gCameraPosition.v[0] = 0;
    }
    if (gCameraPosition.v[0] > MP_GetMapSize() * MP_BLOCK_SIZE) {
        gCameraPosition.v[0] = MP_GetMapSize() * MP_BLOCK_SIZE;
    }

    if (gCameraPosition.v[1] < 0) {
        gCameraPosition.v[1] = 0;
    }
    if (gCameraPosition.v[1] > MP_GetMapSize() * MP_BLOCK_SIZE) {
        gCameraPosition.v[1] = MP_GetMapSize() * MP_BLOCK_SIZE;
    }

    gCameraVelocity.v[0] *= (1 - MP_CAMERA_FRICTION * MP_BLOCK_SIZE / MP_FRAMERATE);
    gCameraVelocity.v[1] *= (1 - MP_CAMERA_FRICTION * MP_BLOCK_SIZE / MP_FRAMERATE);

    if (fabs(gCameraVelocity.v[0]) < 0.0001f) {
        gCameraVelocity.v[0] = 0;
    }
    if (fabs(gCameraVelocity.v[1]) < 0.0001f) {
        gCameraVelocity.v[1] = 0;
    }

    gCameraZoom += (gCameraZoomTarget - gCameraZoom) / 2.0f;

    gCameraPosition.v[2] = MP_CAMERA_HEIGHT - gCameraZoom * MP_CAMERA_MAX_ZOOM;

    gCameraTarget.v[0] = gCameraPosition.v[0];
    gCameraTarget.v[1] = gCameraPosition.v[1] + MP_CAMERA_TARGET_DISTANCE;
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

const vec3* MP_GetCameraPosition(void) {
    return &gCameraPosition;
}

const vec3* MP_GetCameraTarget(void) {
    return &gCameraTarget;
}

float MP_GetCameraZoom(void) {
    return gCameraZoom;
}

void MP_CameraStartScrolling(MP_CameraDirection direction) {
    gCameraDirection |= direction;
}

void MP_CameraStopScrolling(MP_CameraDirection direction) {
    gCameraDirection &= ~direction;
}

void MP_CameraZoomIn(void) {
    gCameraZoomTarget += MP_CAMERA_ZOOM_STEP;
    if (gCameraZoomTarget > 1.0f) {
        gCameraZoomTarget = 1.0f;
    }
}

void MP_CameraZoomOut(void) {
    gCameraZoomTarget -= MP_CAMERA_ZOOM_STEP;
    if (gCameraZoomTarget < 0) {
        gCameraZoomTarget = 0;
    }
}

void MP_InitCamera(void) {
    MP_OnUpdate(update);
}
