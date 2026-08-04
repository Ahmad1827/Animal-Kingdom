#include "simulation/PopulationGenerator.h"
#include "world/TerrainGenerator.h"
#include "world/SeedManager.h"

namespace sim {

ApeData PopulationGenerator::createRandomApe(uint32_t seed, DynastyID dynastyId, const std::vector<std::string>& names, const std::vector<Trait>& traits, uint32_t worldSeed) {
    uint32_t apeSeed = seed;
    ApeData ape;
    ape.id = IDGenerator::generateEntityID();
    ape.dynastyId = dynastyId;
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
        ape.traits.push_back(traits[SeedManager::getRandomInt(apeSeed, 0, traits.size() - 1)]);
    }

    ape.homeChunkX = SeedManager::getRandomInt(apeSeed, -5, 5);
    ape.worldX = (ape.homeChunkX * 2000.0f) + SeedManager::getRandomFloat(apeSeed, 200.0f, 1800.0f);
    ape.worldY = TerrainGenerator::getTerrainHeight(ape.worldX, worldSeed) - 200.0f; 
    ape.currentChunkX = ape.homeChunkX;
    ape.currentChunkY = static_cast<int>(ape.worldY / 2000.0f);

    return ape;
}

EntityID PopulationGenerator::generatePlayerDynasty(SimulationRegistry& registry, uint32_t worldSeed) {
    std::vector<std::string> names = {"Caesar", "Maurice", "Cornelius", "Zira", "Aldo", "Blue Eyes", "Rocket"};
    std::vector<Trait> allTraits = {Trait::Brave, Trait::Strategic, Trait::Honorable};

    DynastyData dyn;
    dyn.id = IDGenerator::generateDynastyID();
    dyn.name = "Kong";
    dyn.wealth = 100;
    dyn.prestige = 50;
    dyn.legitimacy = 100;

    ApeData founder = createRandomApe(worldSeed + 1000, dyn.id, names, allTraits, worldSeed);
    founder.age = 35.0f;
    founder.worldX = 1000.0f;
    founder.worldY = TerrainGenerator::getTerrainHeight(founder.worldX, worldSeed) - 200.0f;
    
    ApeData spouse = createRandomApe(worldSeed + 1001, dyn.id, names, allTraits, worldSeed);
    spouse.age = 32.0f;
    spouse.worldX = founder.worldX + 50.0f;
    spouse.worldY = founder.worldY;
    spouse.spouseId = founder.id;
    founder.spouseId = spouse.id;

    ApeData child = createRandomApe(worldSeed + 1002, dyn.id, names, allTraits, worldSeed);
    child.age = 12.0f;
    child.worldX = founder.worldX - 50.0f;
    child.worldY = founder.worldY;
    child.fatherId = founder.gender == Gender::Male ? founder.id : spouse.id;
    child.motherId = founder.gender == Gender::Female ? founder.id : spouse.id;
    
    ApeData sibling = createRandomApe(worldSeed + 1003, dyn.id, names, allTraits, worldSeed);
    sibling.age = 33.0f;
    sibling.worldX = founder.worldX + 150.0f;
    sibling.worldY = founder.worldY;
    
    founder.children.push_back(child.id);
    spouse.children.push_back(child.id);
    founder.siblings.push_back(sibling.id);
    sibling.siblings.push_back(founder.id);

    dyn.founderId = founder.id;
    dyn.currentLeaderId = founder.id;
    dyn.primaryHeirId = child.id;
    dyn.members = {founder.id, spouse.id, child.id, sibling.id};

    registry.registerDynasty(dyn);
    registry.registerApe(founder);
    registry.registerApe(spouse);
    registry.registerApe(child);
    registry.registerApe(sibling);

    return founder.id;
}

void PopulationGenerator::generateInitialPopulation(SimulationRegistry& registry, uint32_t worldSeed) {
    std::vector<std::string> names = {"Koba", "Ash", "Buck", "Luca", "Spear", "Fang", "Goro", "Kala", "Kerchak", "Terk"};
    std::vector<Trait> allTraits = {Trait::Brave, Trait::Coward, Trait::Greedy, Trait::Cruel, Trait::Lazy};

    uint32_t popSeed = worldSeed; 
    int populationSize = SeedManager::getRandomInt(popSeed, 30, 50);

    for (int i = 0; i < populationSize; ++i) {
        ApeData ape = createRandomApe(worldSeed + (i * 2000), 0, names, allTraits, worldSeed);
        registry.registerApe(ape);
    }
}

}