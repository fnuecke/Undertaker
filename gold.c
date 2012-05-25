#include "gold.h"

#include <malloc.h>

#include "log.h"

typedef struct GoldPile {
    vec2 position;
    unsigned int size;
} GoldPile;

static GoldPile* gGoldPiles = NULL;
static unsigned int gPileCount = 0;
static unsigned int gPileCapacity = 0;

static void ensureCapacity(void) {
    if (gPileCount >= gPileCapacity) {
        gPileCapacity = gPileCapacity * 2 + 1;
        if (!(gGoldPiles = realloc(gGoldPiles, gPileCapacity * sizeof (GoldPile)))) {
            MP_log_fatal("Out of memory resizing gold pile list.\n");
        }
    }
}

