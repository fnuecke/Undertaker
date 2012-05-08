#include "units_meta.h"

META_globals(MP_UnitMeta)

/** Reset defaults on map change */
static void resetDefaults(void) {
    gMetaDefaults.canPass = 0;
    gMetaDefaults.moveSpeed = 1.0f;
}

/** New type registered */
inline static bool initMeta(MP_UnitMeta* m, const MP_UnitMeta* meta) {
    *m = *meta;

    return true;
}

/** Type override */
inline static bool updateMeta(MP_UnitMeta* m, const MP_UnitMeta* meta) {
    m->moveSpeed = meta->moveSpeed;
    m->canPass = meta->canPass;
    m->satisfaction = meta->satisfaction;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(MP_UnitMeta* m) {
}

META_impl(MP_UnitMeta, Unit)
