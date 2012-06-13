#include "meta_unit.h"

#include "meta_impl.h"

///////////////////////////////////////////////////////////////////////////////
// Constants / globals
///////////////////////////////////////////////////////////////////////////////

META_globals(MP_UnitMeta)

///////////////////////////////////////////////////////////////////////////////
// Meta implementation
///////////////////////////////////////////////////////////////////////////////

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
    m->canPass = meta->canPass;
    m->moveSpeed = meta->moveSpeed;
    m->satisfaction = meta->satisfaction;
    m->goldCapacity = meta->goldCapacity;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(MP_UnitMeta* m) {
    free(m->jobs);
    m->jobs = NULL;
    free(m->satisfaction.jobSaturation);
    m->satisfaction.jobSaturation = NULL;
    m->jobCount = 0;
}

META_impl(MP_UnitMeta, Unit)
