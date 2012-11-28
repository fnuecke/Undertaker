#include "passability.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "events.h"
#include "log.h"

#define PASSABILITY_TYPES_MAX (CHAR_BIT * sizeof(MP_Passability))

static char* gPassabilityTypes[PASSABILITY_TYPES_MAX] = {NULL};
static unsigned int gPassabilityTypeCount = 0;

static void onMapChange(void) {
    for (unsigned int i = 0; i < gPassabilityTypeCount; ++i) {
        free(gPassabilityTypes[i]);
        gPassabilityTypes[i] = NULL;
    }
    gPassabilityTypeCount = 0;
}

bool MP_AddPassability(const char* name) {
    // Is the name valid?
    if (!name || !strlen(name)) {
        MP_log_error("Invalid passability type name (null or empty).");
        return false;
    }

    // Too many types.
    if (gPassabilityTypeCount >= PASSABILITY_TYPES_MAX) {
        MP_log_error("Too many passability types (maximum is %d).", PASSABILITY_TYPES_MAX);
        return false;
    }

    // Only do something if we don't already know that type.
    if (!MP_GetPassability(name)) {
        // Copy the string.
        gPassabilityTypes[gPassabilityTypeCount] = calloc(strlen(name) + 1, sizeof (char));
        strcpy(gPassabilityTypes[gPassabilityTypeCount], name);
        ++gPassabilityTypeCount;

        MP_log_info("Registered passability type '%s'.\n", name);
    } else {
        MP_log_info("Redundant passability type '%s'.\n", name);
    }

    return true;
}

MP_Passability MP_GetPassability(const char* name) {
    for (unsigned int i = 0; i < gPassabilityTypeCount; ++i) {
        if (strcmp(name, gPassabilityTypes[i]) == 0) {
            return 1 << i;
        }
    }
    return 0;
}

int MP_Lua_AddPassability(lua_State* L) {
    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_isstring(L, 1), 0, "one 'string' expected");

    // Get the name and store it.
    if (!MP_AddPassability(lua_tostring(L, 1))) {
        luaL_argerror(L, 1, "invalid passability name or too many entries");
    }

    return 0;
}

void MP_InitPassability(void) {
    MP_AddMapChangeEventListener(onMapChange);
}
