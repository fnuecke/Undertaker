#include <inttypes.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>

#include "config.h"
#include "input.h"
#include "map.h"
#include "camera.h"

typedef enum {
    MODE_NONE,
    MODE_SELECT,
    MODE_DESELECT
} SelectMode;

static SelectMode mode;
static int start_x, start_y;

static GLuint pick_object(int x, int y, void(*render)(void)) {
    GLuint buffer[64] = {0};

    glSelectBuffer(sizeof (buffer), buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(0);

    // Now modify the viewing volume, restricting selection area around the cursor
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    // Restrict the draw to an area around the cursor.
    {
        GLint view[4];
        glGetIntegerv(GL_VIEWPORT, view);
        gluPickMatrix(x, y, 1.0, 1.0, view);
        gluPerspective(80.0, DK_ASPECT_RATIO, 0.1, 1000.0);

        render();
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    GLint hits = glRenderMode(GL_RENDER);

    GLuint closest = 0;
    unsigned int hit, depth = UINT32_MAX;
    for (hit = 0; hit < hits; ++hit) {
        if (buffer[hit * 4 + 3] && buffer[hit * 4 + 2] < depth) {
            closest = buffer[hit * 4 + 3];
            depth = buffer[hit * 4 + 1];
        }
    }
    return closest;
}

inline static GLuint pick_block(int x, int y) {
    return pick_object(x, y, &DK_render_map);
}

void DK_key_down(const SDL_Event* e) {
    switch (e->key.keysym.sym) {
        case SDLK_UP:
            DK_camera_set_direction(DK_CAMD_NORTH);
            break;
        case SDLK_DOWN:
            DK_camera_set_direction(DK_CAMD_SOUTH);
            break;
        case SDLK_LEFT:
            DK_camera_set_direction(DK_CAMD_WEST);
            break;
        case SDLK_RIGHT:
            DK_camera_set_direction(DK_CAMD_EAST);
            break;
    }
}

void DK_key_up(const SDL_Event* e) {
    switch (e->key.keysym.sym) {
        case SDLK_UP:
            DK_camera_unset_direction(DK_CAMD_NORTH);
            break;
        case SDLK_DOWN:
            DK_camera_unset_direction(DK_CAMD_SOUTH);
            break;
        case SDLK_LEFT:
            DK_camera_unset_direction(DK_CAMD_WEST);
            break;
        case SDLK_RIGHT:
            DK_camera_unset_direction(DK_CAMD_EAST);
            break;
    }
}

void DK_mouse_down(const SDL_Event* e) {
    switch (e->button.button) {
        case SDL_BUTTON_WHEELUP:
            DK_camera_zoom_in();
            break;
        case SDL_BUTTON_WHEELDOWN:
            DK_camera_zoom_out();
            break;
        case SDL_BUTTON_LEFT:
        {
            // Find the world object we clicked on.
            GLuint name;
            if (name = pick_block(e->button.x, DK_RESOLUTION_Y - e->button.y)) {
                // See if it's a selectable block.
                DK_as_block(name, &start_x, &start_y);
                if (DK_block_is_selectable(DK_PLAYER_RED, start_x, start_y)) {
                    // Yes, if it's a non-empty one, start selection.
                    if (DK_block_is_selected(DK_PLAYER_RED, start_x, start_y)) {
                        mode = MODE_DESELECT;
                    } else {
                        mode = MODE_SELECT;
                    }
                    // Don't test for other objects.
                    return;
                } else {
                    mode = MODE_NONE;
                }
            }

        }
            break;
    }
}

void DK_mouse_up(const SDL_Event* e) {
    switch (e->button.button) {
        case SDL_BUTTON_LEFT:
        {
            if (mode != MODE_NONE) {
                // Find the world object we released over.
                GLuint name;
                int end_x, end_y;
                if (name = pick_block(e->button.x, DK_RESOLUTION_Y - e->button.y)) {
                    DK_as_block(name, &end_x, &end_y);
                } else {
                    // This should not happen... it would mean we can click into
                    // the void (no map rendered at some point of the screen).
                    end_x = start_x;
                    end_y = start_y;
                }

                if (end_x < start_x) {
                    const int tmp = end_x;
                    end_x = start_x;
                    start_x = tmp;
                }
                if (end_y < start_y) {
                    const int tmp = end_y;
                    end_y = start_y;
                    start_y = tmp;
                }

                int x, y;
                for (x = start_x; x <= end_x; ++x) {
                    for (y = start_y; y <= end_y; ++y) {
                        if (mode == MODE_SELECT) {
                            DK_block_select(DK_PLAYER_RED, x, y);
                        } else if (mode == MODE_DESELECT) {
                            DK_block_deselect(DK_PLAYER_RED, x, y);
                        }
                    }
                }
            }
        }
            break;
    }
}
