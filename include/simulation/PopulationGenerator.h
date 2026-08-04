#pragma once
#include "simulation/SimulationRegistry.h"
#include <cstdint>
#include <vector>
#include <string>

namespace sim {

class PopulationGenerator {
private:
    static ApeData createRandomApe(uint32_t seed, DynastyID dynastyId, const std::vector<std::string>& names, const std::vector<Trait>& traits, uint32_t worldSeed);

public:
    static void generateInitialPopulation(SimulationRegistry& registry, uint32_t worldSeed);
    static EntityID generatePlayerDynasty(SimulationRegistry& registry, uint32_t worldSeed);
};

}