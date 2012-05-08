#include "update.h"

#include "events.h"

static Callbacks* gUpdateCallbacks = 0;

void MP_Update(void) {
    CB_Call(gUpdateCallbacks);
}

void MP_OnUpdate(callback method) {
    if (!gUpdateCallbacks) {
        gUpdateCallbacks = CB_New();
    }
    CB_Add(gUpdateCallbacks, method);
}
