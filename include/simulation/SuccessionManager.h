#pragma once
#include "simulation/SimulationRegistry.h"
#include "simulation/EntityID.h"

namespace sim {

class SuccessionManager {
public:
    static EntityID findNextHeir(SimulationRegistry& registry, EntityID currentLeaderId);
};

}