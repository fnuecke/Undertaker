#include "camera.h"
#include "map.h"
#include "units.h"
#include "update.h"

void DK_update(void) {
    DK_UpdateCamera();
    DK_update_map();
    DK_update_units();
}
