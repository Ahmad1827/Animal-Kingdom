#include "simulation/PopulationGenerator.h"
#include "world/TerrainGenerator.h"
#include "world/SeedManager.h"
#include <cmath>

namespace sim {

ApeData PopulationGenerator::createRandomApe(uint32_t seed, DynastyID dynastyId, VillageID villageId, const std::vector<std::string>& names, const std::vector<Trait>& traits, uint32_t worldSeed) {
    uint32_t apeSeed = seed;
    ApeData ape;
    ape.id = IDGenerator::generateEntityID();
    ape.dynastyId = dynastyId;
    ape.villageId = villageId;
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
    ape.currentJob = Job::Idle;
    ape.currentTargetNode = 0;
    ape.currentTargetStructure = 0;
    ape.carriedType = ResourceType::None;
    ape.carriedAmount = 0;
    ape.equippedTool = ToolType::None;

    int traitCount = SeedManager::getRandomInt(apeSeed, 1, 3);
    for(int t = 0; t < traitCount; ++t) {
        ape.traits.push_back(traits[SeedManager::getRandomInt(apeSeed, 0, traits.size() - 1)]);
    }

    return ape;
}

void spawnNodes(SimulationRegistry& registry, float centerX, uint32_t worldSeed) {
    // 3 Food
    for (int i = -1; i <= 1; ++i) {
        ResourceNode res;
        res.id = IDGenerator::generateEntityID();
        res.type = ResourceType::Food;
        res.worldX = centerX + (i * 600.f) + 300.f;
        res.worldY = TerrainGenerator::getTerrainHeight(res.worldX, worldSeed) - 50.f;
        res.amount = 10; res.maxAmount = 10; res.regrowTimer = 0.f;
        registry.registerResource(res);
    }
    // 2 Wood
    for (int i = -1; i <= 1; i+=2) {
        ResourceNode res;
        res.id = IDGenerator::generateEntityID();
        res.type = ResourceType::Wood;
        res.worldX = centerX + (i * 800.f);
        res.worldY = TerrainGenerator::getTerrainHeight(res.worldX, worldSeed) - 50.f;
        res.amount = 20; res.maxAmount = 20; res.regrowTimer = 0.f;
        registry.registerResource(res);
    }
    // 1 Stone
    ResourceNode res;
    res.id = IDGenerator::generateEntityID();
    res.type = ResourceType::Stone;
    res.worldX = centerX + 1200.f;
    res.worldY = TerrainGenerator::getTerrainHeight(res.worldX, worldSeed) - 50.f;
    res.amount = 15; res.maxAmount = 15; res.regrowTimer = 0.f;
    registry.registerResource(res);
}

EntityID PopulationGenerator::generatePlayerDynasty(SimulationRegistry& registry, uint32_t worldSeed) {
    std::vector<std::string> names = {"Caesar", "Maurice", "Cornelius", "Zira", "Aldo", "Blue Eyes", "Rocket"};
    std::vector<Trait> allTraits = {Trait::Brave, Trait::Strategic, Trait::Honorable};

    VillageData pVillage;
    pVillage.id = IDGenerator::generateVillageID();
    pVillage.name = "First Tree";
    pVillage.homeChunkX = 0;
    pVillage.centerX = 1000.0f;
    pVillage.centerY = TerrainGenerator::getTerrainHeight(pVillage.centerX, worldSeed) - 200.0f;
    pVillage.food = 50; pVillage.wood = 20; pVillage.stone = 5;
    pVillage.toolsAxe = 1; pVillage.toolsPick = 0; pVillage.toolsSpear = 0; pVillage.toolsTorch = 0;
    pVillage.territoryRadius = 3000.0f;
    
    // FREEZE PHYSICAL BORDERS AT CREATION
    pVillage.borderMinX = pVillage.centerX - pVillage.territoryRadius;
    pVillage.borderMaxX = pVillage.centerX + pVillage.territoryRadius;

    // Initial Storage Hut
    StructureData s;
    s.id = IDGenerator::generateStructureID();
    s.type = StructureType::StorageHut;
    s.villageId = pVillage.id;
    s.worldX = pVillage.centerX;
    s.worldY = pVillage.centerY;
    s.progress = 60.f; s.maxProgress = 60.f;
    s.reqWood = 30; s.reqStone = 10; s.curWood = 30; s.curStone = 10;
    s.isFinished = true;
    pVillage.finishedStructures.push_back(s.id);
    registry.registerStructure(s);

    DynastyData dyn;
    dyn.id = IDGenerator::generateDynastyID();
    dyn.name = "Kong";
    dyn.wealth = 100; dyn.prestige = 50; dyn.legitimacy = 100;

    ApeData founder = createRandomApe(worldSeed + 1000, dyn.id, pVillage.id, names, allTraits, worldSeed);
    founder.age = 35.0f; founder.worldX = pVillage.centerX; founder.worldY = pVillage.centerY;
    founder.homeX = founder.worldX; founder.homeY = founder.worldY;
    
    ApeData spouse = createRandomApe(worldSeed + 1001, dyn.id, pVillage.id, names, allTraits, worldSeed);
    spouse.age = 32.0f; spouse.worldX = pVillage.centerX + 50.0f; spouse.worldY = pVillage.centerY;
    spouse.homeX = spouse.worldX; spouse.homeY = spouse.worldY; spouse.spouseId = founder.id; founder.spouseId = spouse.id;

    ApeData child = createRandomApe(worldSeed + 1002, dyn.id, pVillage.id, names, allTraits, worldSeed);
    child.age = 12.0f; child.worldX = pVillage.centerX - 50.0f; child.worldY = pVillage.centerY;
    child.homeX = child.worldX; child.homeY = child.worldY;
    child.fatherId = founder.gender == Gender::Male ? founder.id : spouse.id;
    child.motherId = founder.gender == Gender::Female ? founder.id : spouse.id;
    
    ApeData sibling = createRandomApe(worldSeed + 1003, dyn.id, pVillage.id, names, allTraits, worldSeed);
    sibling.age = 33.0f; sibling.worldX = pVillage.centerX + 150.0f; sibling.worldY = pVillage.centerY;
    sibling.homeX = sibling.worldX; sibling.homeY = sibling.worldY;
    
    founder.children.push_back(child.id); spouse.children.push_back(child.id);
    founder.siblings.push_back(sibling.id); sibling.siblings.push_back(founder.id);

    dyn.founderId = founder.id; dyn.currentLeaderId = founder.id; dyn.primaryHeirId = child.id;
    dyn.members = {founder.id, spouse.id, child.id, sibling.id};

    pVillage.founderId = founder.id; pVillage.leaderId = founder.id; pVillage.members = dyn.members;

    registry.registerVillage(pVillage); registry.registerDynasty(dyn);
    registry.registerApe(founder); registry.registerApe(spouse); registry.registerApe(child); registry.registerApe(sibling);

    spawnNodes(registry, pVillage.centerX, worldSeed);

    return founder.id;
}

void PopulationGenerator::generateVillages(SimulationRegistry& registry, uint32_t worldSeed) {
    std::vector<std::string> names = {"Koba", "Ash", "Buck", "Luca", "Spear", "Fang", "Goro", "Kala", "Kerchak", "Terk"};
    std::vector<Trait> allTraits = {Trait::Brave, Trait::Coward, Trait::Greedy, Trait::Cruel, Trait::Lazy};

    uint32_t popSeed = worldSeed;
    int numVillages = SeedManager::getRandomInt(popSeed, 3, 6);

    for (int v = 0; v < numVillages; ++v) {
        VillageData village;
        village.id = IDGenerator::generateVillageID();
        village.name = "Tribe " + std::to_string(v + 1);
        
        int offset = (v % 2 == 0 ? 1 : -1) * (v + 1) * 3;
        village.homeChunkX = offset;
        village.centerX = village.homeChunkX * 2000.f + 1000.f;
        village.centerY = TerrainGenerator::getTerrainHeight(village.centerX, worldSeed) - 200.f;
        village.food = 30; village.wood = 10; village.stone = 5;
        village.toolsAxe = 0; village.toolsPick = 0; village.toolsSpear = 0; village.toolsTorch = 0;
        village.territoryRadius = 2500.f;

        // FREEZE PHYSICAL BORDERS AT CREATION
        village.borderMinX = village.centerX - village.territoryRadius;
        village.borderMaxX = village.centerX + village.territoryRadius;

        StructureData s;
        s.id = IDGenerator::generateStructureID();
        s.type = StructureType::StorageHut;
        s.villageId = village.id;
        s.worldX = village.centerX; s.worldY = village.centerY;
        s.progress = 60.f; s.maxProgress = 60.f;
        s.reqWood = 30; s.reqStone = 10; s.curWood = 30; s.curStone = 10;
        s.isFinished = true;
        village.finishedStructures.push_back(s.id);
        registry.registerStructure(s);

        int pop = SeedManager::getRandomInt(popSeed, 5, 12);
        for (int i = 0; i < pop; ++i) {
            ApeData ape = createRandomApe(worldSeed + (v * 1000) + i, 0, village.id, names, allTraits, worldSeed);
            ape.worldX = village.centerX + SeedManager::getRandomFloat(popSeed, -200.f, 200.f);
            ape.worldY = village.centerY;
            ape.homeX = ape.worldX; ape.homeY = ape.worldY;
            if (i == 0) { village.founderId = ape.id; village.leaderId = ape.id; }
            village.members.push_back(ape.id);
            registry.registerApe(ape);
        }
        registry.registerVillage(village);
        spawnNodes(registry, village.centerX, worldSeed);
    }
}
}