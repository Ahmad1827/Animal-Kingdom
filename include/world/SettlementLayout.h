#pragma once
#include <cstdint>
#include <vector>

// Single source of truth for where settlements sit in the world.
//
// This existed in two places before: WorldGenerator::getClearanceZones() used
// `3 + (worldSeed % 3)` while PopulationGenerator::generateVillages() used
// SeedManager::getRandomInt(popSeed, 3, 5). Those two produce DIFFERENT counts
// from the same seed, so any village beyond the clearance count got full jungle
// tree generation dropped on top of it. Both callers now use this.
namespace SettlementLayout {

    // The player's home settlement (First Tree). Fixed.
    float getPlayerCenterX();
    float getPlayerTerritoryRadius();

    // Territory radius used by the AI villages.
    float getVillageTerritoryRadius();

    // How many AI villages this seed produces. Consumes nothing global.
    int getVillageCount(uint32_t worldSeed);

    // Centre X of each AI village, in generation order (NOT sorted).
    // Index i here == village index i in PopulationGenerator::generateVillages().
    std::vector<float> getVillageCenters(uint32_t worldSeed);
}