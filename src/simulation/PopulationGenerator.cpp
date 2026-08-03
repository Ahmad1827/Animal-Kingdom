#include "simulation/PopulationGenerator.h"
#include "world/TerrainGenerator.h"
#include "world/SeedManager.h"
#include <vector>
#include <string>

namespace sim {

void PopulationGenerator::generateInitialPopulation(SimulationRegistry& registry, uint32_t worldSeed) {
    std::vector<std::string> names = {
        "Koba", "Caesar", "Maurice", "Cornelius", "Zira", "Aldo", "Blue Eyes", "Ash", "Rocket", "Buck", 
        "Luca", "Spear", "Fang", "Goro", "Kala", "Kerchak", "Terk", "Mojo", "Rafiki", "Kong"
    };

    std::vector<Trait> allTraits = {
        Trait::Brave, Trait::Coward, Trait::Greedy, Trait::Honorable, Trait::Cruel, 
        Trait::Charismatic, Trait::Lazy, Trait::Strategic, Trait::Impulsive, Trait::Curious, Trait::Energetic
    };

    uint32_t popSeed = worldSeed; // Local mutable copy of the seed
    int populationSize = SeedManager::getRandomInt(popSeed, 30, 50);

    for (int i = 0; i < populationSize; ++i) {
        // Create a unique mutable seed for this specific ape
        uint32_t apeSeed = worldSeed + (i * 1000); 
        
        ApeData ape;
        ape.id = IDGenerator::generateEntityID();
        ape.dynastyId = 0; // No dynasty mechanics yet
        
        // Pass the mutable apeSeed so it steps forward automatically
        ape.name = names[SeedManager::getRandomInt(apeSeed, 0, names.size() - 1)];
        ape.age = SeedManager::getRandomFloat(apeSeed, 5.0f, 40.0f);
        ape.gender = (SeedManager::getRandomInt(apeSeed, 0, 1) == 0) ? Gender::Male : Gender::Female;
        ape.health = 100.0f;
        ape.hunger = 100.0f;
        ape.alive = true;
        ape.birthDateTicks = 0;
        ape.currentKingdom = 0;
        ape.currentOccupation = Occupation::Unemployed;
        ape.currentGoal = Goal::Survive;

        int traitCount = SeedManager::getRandomInt(apeSeed, 1, 3);
        for(int t = 0; t < traitCount; ++t) {
            ape.traits.push_back(allTraits[SeedManager::getRandomInt(apeSeed, 0, allTraits.size() - 1)]);
        }

        // Spawn roughly near the starting area (Chunks -5 to 5)
        ape.homeChunkX = SeedManager::getRandomInt(apeSeed, -5, 5);
        ape.worldX = (ape.homeChunkX * 2000.0f) + SeedManager::getRandomFloat(apeSeed, 200.0f, 1800.0f);
        
        // TerrainGenerator likely takes worldSeed by value/const, so we leave it as worldSeed
        ape.worldY = TerrainGenerator::getTerrainHeight(ape.worldX, worldSeed) - 200.0f; 
        
        ape.currentChunkX = ape.homeChunkX;
        ape.currentChunkY = static_cast<int>(ape.worldY / 2000.0f);

        registry.registerApe(ape);
    }
}

}