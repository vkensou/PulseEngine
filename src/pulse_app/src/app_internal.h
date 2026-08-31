#pragma once

#include <flecs.h>
#include "pulse_app.h"

struct pulse_app_state_resource {
    PulseAppId app;
};

extern ECS_COMPONENT_DECLARE(PulseTimer);
extern ECS_COMPONENT_DECLARE(pulse_app_state_resource);
