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
    ape.age = SeedManager::getRandomFloat(apeSeed, 14.0f, 42.0f);
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

static void spawnBaseStructures(SimulationRegistry& registry, VillageData& village) {
    auto addStruct = [&](StructureType type, float offsetX, int wood, int stone) {
        StructureData s;
        s.id = IDGenerator::generateStructureID();
        s.type = type;
        s.villageId = village.id;
        s.worldX = village.centerX + offsetX;
        s.worldY = 500.0f;
        s.progress = 60.f;
        s.maxProgress = 60.f;
        s.reqWood = wood;
        s.reqStone = stone;
        s.curWood = wood;
        s.curStone = stone;
        s.isFinished = true;
        village.finishedStructures.push_back(s.id);
        registry.registerStructure(s);
    };

    addStruct(StructureType::VillageCenter, 0.f, 50, 20);
    addStruct(StructureType::Bonfire, 65.f, 10, 5);
    addStruct(StructureType::StorageHut, 185.f, 35, 15);
    addStruct(StructureType::WoodPile, 245.f, 15, 0);
    addStruct(StructureType::StonePile, 275.f, 0, 15);
    addStruct(StructureType::Nest, -145.f, 20, 5);
    addStruct(StructureType::Nest, -205.f, 20, 5);
    addStruct(StructureType::BuilderHut, -285.f, 30, 10);
    addStruct(StructureType::WatchPlatform, 340.f, 25, 10);
    addStruct(StructureType::SimpleBarrier, -360.f, 15, 0);
    addStruct(StructureType::SimpleBarrier, 400.f, 15, 0);
}

void spawnNodes(SimulationRegistry& registry, float centerX, uint32_t worldSeed) {
    for (int i = -1; i <= 1; ++i) {
        ResourceNode res;
        res.id = IDGenerator::generateEntityID();
        res.type = ResourceType::Food;
        res.worldX = centerX + (i * 700.f) + (i >= 0 ? 550.f : -550.f);
        res.worldY = 500.0f - 50.f;
        res.amount = 12;
        res.maxAmount = 12;
        res.regrowTimer = 0.f;
        registry.registerResource(res);
    }
    for (int i = -1; i <= 1; i += 2) {
        ResourceNode res;
        res.id = IDGenerator::generateEntityID();
        res.type = ResourceType::Wood;
        res.worldX = centerX + (i * 950.f);
        res.worldY = 500.0f - 50.f;
        res.amount = 25;
        res.maxAmount = 25;
        res.regrowTimer = 0.f;
        registry.registerResource(res);
    }
    ResourceNode res;
    res.id = IDGenerator::generateEntityID();
    res.type = ResourceType::Stone;
    res.worldX = centerX + 1300.f;
    res.worldY = 500.0f - 50.f;
    res.amount = 20;
    res.maxAmount = 20;
    res.regrowTimer = 0.f;
    registry.registerResource(res);
}

EntityID PopulationGenerator::generatePlayerDynasty(SimulationRegistry& registry, uint32_t worldSeed) {
    std::vector<std::string> names = {"Caesar", "Maurice", "Cornelius", "Zira", "Aldo", "Blue Eyes", "Rocket", "Nova", "Luca"};
    std::vector<Trait> allTraits = {Trait::Brave, Trait::Strategic, Trait::Honorable, Trait::Charismatic};

    VillageData pVillage;
    pVillage.id = IDGenerator::generateVillageID();
    pVillage.name = "First Tree";
    pVillage.identity = VillageIdentity::Balanced;
    pVillage.homeChunkX = 0;
    pVillage.centerX = 1000.0f;
    pVillage.centerY = 500.0f;
    pVillage.food = 65;
    pVillage.wood = 35;
    pVillage.stone = 15;
    pVillage.toolsAxe = 2;
    pVillage.toolsPick = 1;
    pVillage.toolsSpear = 2;
    pVillage.toolsBasket = 2;
    pVillage.toolsTorch = 1;
    pVillage.territoryRadius = 3000.0f;
    pVillage.borderMinX = pVillage.centerX - pVillage.territoryRadius;
    pVillage.borderMaxX = pVillage.centerX + pVillage.territoryRadius;

    spawnBaseStructures(registry, pVillage);

    DynastyData dyn;
    dyn.id = IDGenerator::generateDynastyID();
    dyn.name = "Kong";
    dyn.wealth = 100;
    dyn.prestige = 80;
    dyn.legitimacy = 100;

    ApeData founder = createRandomApe(worldSeed + 1000, dyn.id, pVillage.id, names, allTraits, worldSeed);
    founder.name = "Koba";
    founder.age = 36.0f;
    founder.worldX = pVillage.centerX;
    founder.worldY = 500.0f;
    founder.homeX = founder.worldX;
    founder.homeY = founder.worldY;
    founder.currentJob = Job::Idle;

    ApeData spouse = createRandomApe(worldSeed + 1001, dyn.id, pVillage.id, names, allTraits, worldSeed);
    spouse.name = "Maya";
    spouse.age = 32.0f;
    spouse.gender = Gender::Female;
    spouse.worldX = pVillage.centerX + 55.0f;
    spouse.worldY = 500.0f;
    spouse.homeX = spouse.worldX;
    spouse.homeY = spouse.worldY;
    spouse.spouseId = founder.id;
    founder.spouseId = spouse.id;
    spouse.currentJob = Job::Forage;

    ApeData child = createRandomApe(worldSeed + 1002, dyn.id, pVillage.id, names, allTraits, worldSeed);
    child.name = "Tano";
    child.age = 15.0f;
    child.gender = Gender::Male;
    child.worldX = pVillage.centerX - 145.0f;
    child.worldY = 500.0f;
    child.homeX = child.worldX;
    child.homeY = child.worldY;
    child.fatherId = founder.id;
    child.motherId = spouse.id;
    child.currentJob = Job::Sleep;

    ApeData sibling = createRandomApe(worldSeed + 1003, dyn.id, pVillage.id, names, allTraits, worldSeed);
    sibling.name = "Boro";
    sibling.age = 42.0f;
    sibling.gender = Gender::Male;
    sibling.worldX = pVillage.centerX + 340.0f;
    sibling.worldY = 500.0f;
    sibling.homeX = sibling.worldX;
    sibling.homeY = sibling.worldY;
    sibling.currentJob = Job::Guard;

    ApeData worker1 = createRandomApe(worldSeed + 1004, dyn.id, pVillage.id, names, allTraits, worldSeed);
    worker1.name = "Aldo";
    worker1.age = 28.0f;
    worker1.worldX = pVillage.centerX - 285.0f;
    worker1.worldY = 500.0f;
    worker1.homeX = worker1.worldX;
    worker1.homeY = worker1.worldY;
    worker1.currentJob = Job::Builder;

    ApeData worker2 = createRandomApe(worldSeed + 1005, dyn.id, pVillage.id, names, allTraits, worldSeed);
    worker2.name = "Maurice";
    worker2.age = 30.0f;
    worker2.worldX = pVillage.centerX + 185.0f;
    worker2.worldY = 500.0f;
    worker2.homeX = worker2.worldX;
    worker2.homeY = worker2.worldY;
    worker2.currentJob = Job::Woodcutter;

    founder.children.push_back(child.id);
    spouse.children.push_back(child.id);
    founder.siblings.push_back(sibling.id);
    sibling.siblings.push_back(founder.id);

    dyn.founderId = founder.id;
    dyn.currentLeaderId = founder.id;
    dyn.primaryHeirId = child.id;
    dyn.members = {founder.id, spouse.id, child.id, sibling.id, worker1.id, worker2.id};

    pVillage.founderId = founder.id;
    pVillage.leaderId = founder.id;
    pVillage.members = dyn.members;

    registry.registerVillage(pVillage);
    registry.registerDynasty(dyn);
    registry.registerApe(founder);
    registry.registerApe(spouse);
    registry.registerApe(child);
    registry.registerApe(sibling);
    registry.registerApe(worker1);
    registry.registerApe(worker2);

    spawnNodes(registry, pVillage.centerX, worldSeed);

    return founder.id;
}

void PopulationGenerator::generateVillages(SimulationRegistry& registry, uint32_t worldSeed) {
    std::vector<std::string> names = {"Spear", "Fang", "Goro", "Kala", "Kerchak", "Terk", "Ash", "Buck", "Brutus", "Goliath"};
    std::vector<Trait> allTraits = {Trait::Brave, Trait::Coward, Trait::Greedy, Trait::Cruel, Trait::Lazy, Trait::Strategic};
    std::vector<VillageIdentity> identities = {VillageIdentity::Aggressive, VillageIdentity::FoodRich, VillageIdentity::StoneFocused, VillageIdentity::WoodFocused, VillageIdentity::Peaceful};

    uint32_t popSeed = worldSeed;
    int numVillages = SeedManager::getRandomInt(popSeed, 3, 5);

    for (int v = 0; v < numVillages; ++v) {
        VillageData village;
        village.id = IDGenerator::generateVillageID();
        village.name = "Tribe of " + names[v % names.size()];
        village.identity = identities[v % identities.size()];
        
        int offset = (v % 2 == 0 ? 1 : -1) * (v + 1) * 3;
        village.homeChunkX = offset;
        village.centerX = village.homeChunkX * 2000.f + 1000.f;
        village.centerY = 500.0f;
        village.food = 40;
        village.wood = 20;
        village.stone = 10;
        village.toolsAxe = 1;
        village.toolsSpear = 2;
        village.territoryRadius = 2500.f;
        village.borderMinX = village.centerX - village.territoryRadius;
        village.borderMaxX = village.centerX + village.territoryRadius;

        spawnBaseStructures(registry, village);

        int pop = SeedManager::getRandomInt(popSeed, 5, 8);
        for (int i = 0; i < pop; ++i) {
            ApeData ape = createRandomApe(worldSeed + (v * 1000) + i, 0, village.id, names, allTraits, worldSeed);
            float xOffset = 0.f;
            if (i == 0) {
                ape.currentJob = Job::Idle;
                xOffset = 0.f;
                village.founderId = ape.id;
                village.leaderId = ape.id;
            } else if (i == 1) {
                ape.currentJob = Job::Guard;
                xOffset = 340.f;
            } else if (i == 2) {
                ape.currentJob = Job::Builder;
                xOffset = -285.f;
            } else if (i == 3) {
                ape.currentJob = Job::Forage;
                xOffset = 185.f;
            } else if (i == 4) {
                ape.currentJob = Job::Sleep;
                xOffset = -145.f;
            } else {
                ape.currentJob = Job::Socialize;
                xOffset = (i % 2 == 0 ? 65.f : -65.f);
            }

            ape.worldX = village.centerX + xOffset;
            ape.worldY = 500.0f;
            ape.homeX = ape.worldX;
            ape.homeY = ape.worldY;

            village.members.push_back(ape.id);
            registry.registerApe(ape);
        }
        registry.registerVillage(village);
        spawnNodes(registry, village.centerX, worldSeed);
    }
}

}