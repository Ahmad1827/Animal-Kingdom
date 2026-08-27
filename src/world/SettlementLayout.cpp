#include "world/SettlementLayout.h"
#include "world/SeedManager.h"

namespace SettlementLayout {

float getPlayerCenterX() { return 1000.0f; }
float getPlayerTerritoryRadius() { return 3000.0f; }
float getVillageTerritoryRadius() { return 2500.0f; }

int getVillageCount(uint32_t worldSeed) {
    // Mirrors exactly what PopulationGenerator used to do inline, so the
    // village count and the tree-clearance count can never drift apart again.
    uint32_t popSeed = worldSeed;
    return SeedManager::getRandomInt(popSeed, 3, 5);
}

std::vector<float> getVillageCenters(uint32_t worldSeed) {
    std::vector<float> centers;
    int count = getVillageCount(worldSeed);
    centers.reserve(count);
    for (int v = 0; v < count; ++v) {
        // Unchanged placement formula - existing worlds keep their layout.
        int offset = (v % 2 == 0 ? 1 : -1) * (v + 1) * 3;
        centers.push_back(offset * 2000.0f + 1000.0f);
    }
    return centers;
}

}