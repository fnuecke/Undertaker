#include "update.h"

#include "events.h"

static Callbacks* gUpdateCallbacks = 0;

void DK_Update(void) {
    CB_Call(gUpdateCallbacks);
}

void DK_OnUpdate(callback method) {
    if (!gUpdateCallbacks) {
        gUpdateCallbacks = CB_New();
    }
    CB_Add(gUpdateCallbacks, method);
}
