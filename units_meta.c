#include "units_meta.h"

META_globals(DK_UnitMeta)

/** Reset defaults on map change */
static void resetDefaults(void) {
    gMetaDefaults.canPass = 0;
    gMetaDefaults.moveSpeed = 1.0f;
}

/** New type registered */
inline static bool initMeta(DK_UnitMeta* m, const DK_UnitMeta* meta) {
    *m = *meta;

    return true;
}

/** Type override */
inline static bool updateMeta(DK_UnitMeta* m, const DK_UnitMeta* meta) {
    m->moveSpeed = meta->moveSpeed;
    m->canPass = meta->canPass;
    m->satisfaction = meta->satisfaction;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(DK_UnitMeta* m) {
}

META_impl(DK_UnitMeta, Unit)
