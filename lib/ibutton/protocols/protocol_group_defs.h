#pragma once

#include "protocol_group_base.h"

typedef enum {
    iButtonProtocolGroupDallas,
    iButtonProtocolGroupMisc,
    iButtonProtocolGroupMax
} iButtonProtocolGroup;

extern const iButtonProtocolGroupBase* const ibutton_protocol_groups[];
