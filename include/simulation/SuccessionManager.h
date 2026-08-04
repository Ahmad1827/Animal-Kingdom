#pragma once
#include "simulation/EntityID.h"

namespace sim {

class SimulationRegistry;

class SuccessionManager {
public:
    static EntityID findNextHeir(SimulationRegistry& registry, EntityID currentLeaderId);
    static void handleDeath(SimulationRegistry& registry, EntityID deceasedId);
};

}