#include "camera.h"
#include "map.h"
#include "units.h"
#include "update.h"

void DK_Update(void) {
    DK_UpdateCamera();
    DK_UpdateMap();
    DK_UpdateUnits();
}
