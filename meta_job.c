#include "meta_job.h"

#include "meta_impl.h"

///////////////////////////////////////////////////////////////////////////////
// Constants and globals
///////////////////////////////////////////////////////////////////////////////

META_globals(MP_JobMeta)

///////////////////////////////////////////////////////////////////////////////
// Init / Update / Teardown
///////////////////////////////////////////////////////////////////////////////

/** Reset defaults on map change */
static void resetDefaults(void) {
    for (unsigned eventId = 0; eventId < MP_JOB_EVENT_COUNT; ++eventId) {
        gMetaDefaults.handlesEvent[eventId] = false;
    }
    gMetaDefaults.hasDynamicPreference = false;
    gMetaDefaults.hasRunMethod = false;
}

/** New type registered */
inline static bool initMeta(MP_JobMeta* m, const MP_JobMeta* meta) {
    *m = *meta;

    return true;
}

/** Type override */
inline static bool updateMeta(MP_JobMeta* m, const MP_JobMeta* meta) {
    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(MP_JobMeta* m) {
}

META_impl(MP_JobMeta, Job)

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

void MP_DisableJobEvent(const MP_JobMeta* meta, MP_JobEvent event) {
    if (meta) {
        // Get non-const pointer...
        for (unsigned int i = 0; i < gMetaCount; ++i) {
            if (i == meta->id - 1) {
                // Modify.
                gMetas[i]->handlesEvent[event] = false;
                MP_log_info("Disabling '%s' for job '%s'.\n", JOB_EVENT_NAME[event], meta->name);
                return;
            }
        }
    }
}

void MP_DisableDynamicPreference(const MP_JobMeta* meta) {
    if (meta) {
        // Get non-const pointer...
        for (unsigned int i = 0; i < gMetaCount; ++i) {
            if (i == meta->id - 1) {
                // Modify.
                gMetas[i]->hasDynamicPreference = false;
                MP_log_info("Disabling 'preference' for job '%s'.\n", meta->name);
                return;
            }
        }
    }
}

void MP_DisableRunMethod(const MP_JobMeta* meta) {
    if (meta) {
        // Get non-const pointer...
        for (unsigned int i = 0; i < gMetaCount; ++i) {
            if (i == meta->id - 1) {
                // Modify.
                gMetas[i]->hasRunMethod = false;
                MP_log_info("Disabling 'run' for job '%s'.\n", meta->name);
                return;
            }
        }
    }
}
