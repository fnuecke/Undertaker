/* 
 * Author: fnuecke
 *
 * Created on April 16, 2012, 1:25 PM
 */

#ifndef INPUT_H
#define	INPUT_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define MP_EVENT(NAME, ...) \
typedef void (*MP_##NAME##EventCallback)(__VA_ARGS__); \
void MP_Add##NAME##EventListener(MP_##NAME##EventCallback callback); \
void MP_Dispatch##NAME##Event(__VA_ARGS__)

    MP_EVENT(Update, void);

    MP_EVENT(MapChange, void);

    MP_EVENT(PreRender, void);

    MP_EVENT(Render, void);

    MP_EVENT(PostRender, void);

    MP_EVENT(ModelMatrixChanged, void);

    MP_EVENT(UnitAdded, MP_Unit*);

    MP_EVENT(BlockTypeChanged, MP_Block*);

    MP_EVENT(BlockOwnerChanged, MP_Block*);

    MP_EVENT(BlockSelectionChanged, MP_Block*, MP_Player);

#undef MP_EVENT

    /**
     * Handle SDL events.
     */
    void MP_Input(void);

#ifdef	__cplusplus
}
#endif

#endif
