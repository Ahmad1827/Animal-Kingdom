#pragma once
#include "simulation/SimulationRegistry.h"

namespace sim {
class JobSystem {
private:
    static EntityID findNearestFoodNode(SimulationRegistry& registry, float x, float y);
    static void assignLeaderJobs(SimulationRegistry& registry, VillageData& village);
public:
    static void updateJobs(SimulationRegistry& registry, float timeOfDay);
};
}