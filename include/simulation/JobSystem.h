#pragma once
#include "simulation/SimulationRegistry.h"

namespace sim {
class JobSystem {
private:
    static EntityID findNearestNode(SimulationRegistry& registry, float x, float y, ResourceType type, VillageData* village);
    static void villagePlanningAI(SimulationRegistry& registry, VillageData& village, uint64_t ticks);
    static void handleLeader(SimulationRegistry& registry, ApeData& leader, VillageData& village);
    static void spawnStructure(SimulationRegistry& registry, VillageData& village, StructureType type);
    static void updateReputations(SimulationRegistry& registry, uint64_t ticks);
public:
    static void updateJobs(SimulationRegistry& registry, float timeOfDay, uint64_t totalTicks);
};
}