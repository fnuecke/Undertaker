#include "meta_ability.h"

#include "meta_impl.h"

///////////////////////////////////////////////////////////////////////////////
// Constants and globals
///////////////////////////////////////////////////////////////////////////////

META_globals(MP_AbilityMeta)

///////////////////////////////////////////////////////////////////////////////
// Init / Update / Teardown
///////////////////////////////////////////////////////////////////////////////

/** Reset defaults on map change */
static void resetDefaults(void) {
    gMetaDefaults.cooldown = 0;
}

/** New type registered */
inline static bool initMeta(MP_AbilityMeta* m, const MP_AbilityMeta* meta) {
    *m = *meta;

    return true;
}

/** Type override */
inline static bool updateMeta(MP_AbilityMeta* m, const MP_AbilityMeta* meta) {
    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(MP_AbilityMeta* m) {
}

META_impl(MP_AbilityMeta, Ability)
