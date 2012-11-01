#include "unit_type.h"

#include "type_impl.h"

///////////////////////////////////////////////////////////////////////////////
// Constants / globals
///////////////////////////////////////////////////////////////////////////////

TYPE_GLOBALS(MP_UnitType)

///////////////////////////////////////////////////////////////////////////////
// Type implementation
///////////////////////////////////////////////////////////////////////////////

/** Reset defaults on map change */
static void resetDefaults(void) {
    gTypeDefaults.canPass = 0;
    gTypeDefaults.moveSpeed = 1.0f;
}

/** New type registered */
inline static bool initType(MP_UnitType* stored, const MP_UnitType* input) {
    *stored = *input;

    return true;
}

/** Type override */
inline static bool updateType(MP_UnitType* stored, const MP_UnitType* input) {
    stored->canPass = input->canPass;
    stored->moveSpeed = input->moveSpeed;

    // TODO merge jobs
    // TODO merge abilities
    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteType(MP_UnitType* type) {
    free(type->jobs);
    type->jobs = NULL;
    type->jobCount = 0;
    free(type->abilities);
    type->abilities = 0;
    type->abilityCount = 0;
}

TYPE_IMPL(MP_UnitType, Unit)
