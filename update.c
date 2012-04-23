#include "camera.h"
#include "map.h"
#include "units.h"

void DK_update(void) {
    DK_update_camera();
    DK_update_map();
    DK_update_units();
}
