#pragma once
#include "simulation/SimulationRegistry.h"
#include <cstdint>

namespace sim {

class PopulationGenerator {
public:
    static void generateInitialPopulation(SimulationRegistry& registry, uint32_t worldSeed);
};

}