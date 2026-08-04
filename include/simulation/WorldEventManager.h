#pragma once
#include "simulation/SimulationRegistry.h"

namespace sim {

class WorldEventManager {
public:
    static void update(SimulationRegistry& registry, uint64_t ticks);
    static float getFoodGrowthModifier(SimulationRegistry& registry, float x);
    static float getWeatherModifier(SimulationRegistry& registry, float x);
private:
    static void handleSeasons(SimulationRegistry& registry, uint64_t ticks);
    static void generateRandomEvents(SimulationRegistry& registry, uint64_t ticks);
    static void updateDiseases(SimulationRegistry& registry, uint64_t ticks);
    static void updateAnimals(SimulationRegistry& registry, uint64_t ticks);
};

}