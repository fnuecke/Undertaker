#include "passability.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "events.h"

#define PASSABILITY_TYPES_MAX (CHAR_BIT * sizeof(DK_Passability))

static char* gPassabilityTypes[PASSABILITY_TYPES_MAX] = {NULL};
static unsigned int gPassabilityTypeCount = 0;

static void onMapSizeChange(void) {
    for (unsigned int i = 0; i < gPassabilityTypeCount; ++i) {
        free(gPassabilityTypes[i]);
        gPassabilityTypes[i] = NULL;
    }
    gPassabilityTypeCount = 0;
}

bool DK_AddPassability(const char* name) {
    // Is the name valid?
    if (!name || !strlen(name)) {
        fprintf(DK_log_target, "ERROR: Invalid passability type name (null or empty).");
        return false;
    }

    // Too many types.
    if (gPassabilityTypeCount >= PASSABILITY_TYPES_MAX) {
        fprintf(DK_log_target, "ERROR: Too many passability types (maximum is %d).", PASSABILITY_TYPES_MAX);
        return false;
    }

    // Only do something if we don't already know that type.
    if (!DK_GetPassability(name)) {
        // Copy the string.
        gPassabilityTypes[gPassabilityTypeCount] = calloc(strlen(name) + 1, sizeof (char));
        strcpy(gPassabilityTypes[gPassabilityTypeCount], name);
        ++gPassabilityTypeCount;

        fprintf(DK_log_target, "INFO: Registered passability type '%s'.\n", name);
    } else {
        fprintf(DK_log_target, "INFO: Redundant passability type '%s'.\n", name);
    }

    return true;
}

DK_Passability DK_GetPassability(const char* name) {
    for (unsigned int i = 0; i < gPassabilityTypeCount; ++i) {
        if (strcmp(name, gPassabilityTypes[i]) == 0) {
            return 1 << i;
        }
    }
    return 0;
}

int DK_Lua_AddPassability(lua_State* L) {
    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_isstring(L, 1), 0, "must specify one string");

    // Get the name and store it.
    if (!DK_AddPassability(luaL_checkstring(L, 1))) {
        luaL_argerror(L, 1, "invalid passability name or too many");
    }

    return 0;
}

void DK_InitPassability(void) {
    DK_OnMapSizeChange(onMapSizeChange);
}
