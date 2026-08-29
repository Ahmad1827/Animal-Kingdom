#include "world/SettlementLayout.h"
#include "world/SeedManager.h"

namespace SettlementLayout {

float getPlayerCenterX() { return 1000.0f; }
float getPlayerTerritoryRadius() { return 1500.0f; }
float getVillageTerritoryRadius() { return 1500.0f; }

int getVillageCount(uint32_t worldSeed) {
    uint32_t popSeed = worldSeed;
    return SeedManager::getRandomInt(popSeed, 3, 5);
}

std::vector<float> getVillageCenters(uint32_t worldSeed) {
    std::vector<float> centers;
    int count = getVillageCount(worldSeed);
    centers.reserve(count);
    for (int v = 0; v < count; ++v) {
        int step = (v / 2) + 1;
        int dir = (v % 2 == 0) ? 1 : -1;
        centers.push_back(1000.0f + static_cast<float>(dir * step) * 12000.0f);
    }
    return centers;
}

}