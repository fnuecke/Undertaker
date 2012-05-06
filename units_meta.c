#include "units_meta.h"

static void initMeta(DK_UnitMeta* m, const DK_UnitMeta* meta) {
    *m = *meta;
}

static void updateMeta(DK_UnitMeta* m, const DK_UnitMeta* meta) {
    m->moveSpeed = meta->moveSpeed;
    m->passability = meta->passability;
    m->satisfaction = meta->satisfaction;
}

META_impl(DK_UnitMeta, Unit)
