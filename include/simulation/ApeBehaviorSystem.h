#pragma once
#include "simulation/SimulationRegistry.h"

namespace sim {

class ApeBehaviorSystem {
public:
    static void updateApeRoleRoutine(ApeData& ape, VillageData& village, SimulationRegistry& registry, float dt, float timeOfDay);
};

}