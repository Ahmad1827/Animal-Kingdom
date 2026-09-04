#include "simulation/PopulationGenerator.h"
#include "world/TerrainGenerator.h"
#include "world/SeedManager.h"
#include "world/SettlementLayout.h"
#include <cmath>
#include <algorithm>
#include "simulation/NameGenerator.h"

namespace sim {

static constexpr float WALL_STAKE_HALF_WIDTH = 104.0f;
static constexpr float WALL_TO_BORDER_OFFSET = WALL_STAKE_HALF_WIDTH * 2.0f;

ApeData PopulationGenerator::createRandomApe(uint32_t seed, DynastyID dynastyId, VillageID villageId, const std::vector<std::string>&, const std::vector<Trait>& traits, uint32_t worldSeed) {
    uint32_t apeSeed = seed;
    ApeData ape;
    ape.id = IDGenerator::generateEntityID();
    ape.dynastyId = dynastyId;
    ape.villageId = villageId;
    ape.isMainApe = (dynastyId != 0);
    ape.councilRole = CouncilRole::None;
    ape.name = sim::NameGenerator::generateUniqueHistoricalName(seed);
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
    ape.amberCount = 0;
    ape.maxAmber = 40;
    ape.currentDisease = DiseaseType::None;
    ape.diseaseSeverity = 0.0f;

    int traitCount = SeedManager::getRandomInt(apeSeed, 1, 3);
    for (int t = 0; t < traitCount; ++t) {
        ape.traits.push_back(traits[SeedManager::getRandomInt(apeSeed, 0, traits.size() - 1)]);
    }

    return ape;
}

static void spawnBaseStructures(SimulationRegistry& registry, VillageData& village) {
    auto addFinished = [&](StructureType type, const std::string& name, float offsetX, const std::string& benefit) {
        StructureData s;
        s.id = IDGenerator::generateStructureID();
        s.type = type;
        s.name = name;
        s.villageId = village.id;
        s.worldX = village.centerX + offsetX;
        s.worldY = 500.0f;
        s.progress = 50.f;
        s.maxProgress = 50.f;
        s.reqWood = 0;
        s.reqStone = 0;
        s.curWood = 0;
        s.curStone = 0;
        s.tier = 1;
        s.requiredAmber = 0;
        s.requiredWood = 0;
        s.requiredStone = 0;
        s.axeCount = 0;
        s.claimedAxes = 0;
        s.isPlanned = false;
        s.isUnderConstruction = false;
        s.isFinished = true;
        s.benefitText = benefit;
        village.finishedStructures.push_back(s.id);
        registry.registerStructure(s);
    };

    auto addBuildPlot = [&](float offsetX) {
        StructureData s;
        s.id = IDGenerator::generateStructureID();
        s.type = StructureType::EmptyPlot;
        s.name = "Empty Building Plot";
        s.villageId = village.id;
        s.worldX = village.centerX + offsetX;
        s.worldY = 500.0f;
        s.progress = 0.f;
        s.maxProgress = 20.f;
        s.reqWood = 0;
        s.reqStone = 0;
        s.curWood = 0;
        s.curStone = 0;
        s.tier = 1;
        s.requiredAmber = 0;
        s.requiredWood = 0;
        s.requiredStone = 0;
        s.axeCount = 0;
        s.claimedAxes = 0;
        s.isPlanned = true;
        s.isUnderConstruction = false;
        s.isFinished = false;
        s.benefitText = "Unclaimed ground for expansion";
        registry.registerStructure(s);
    };

    addFinished(StructureType::VillageCenter, "Clan Hearth & Great Lodge", 0.f, "Seat of the Clan & Ruling Authority");
    addFinished(StructureType::Throne, "Chieftain's Throne", 0.f, "Throne of Ruling Authority");
    addFinished(StructureType::ToolRack, "Tool Rack", -700.f, "Stores Tools for Clan Workers");
    addFinished(StructureType::WoodPile, "Timber Stockpile", 700.f, "Stores Harvested Wood");
    addFinished(StructureType::SimpleBarrier, "West Palisade Wall", -1800.f, "Settlement Palisade Barrier");
    addFinished(StructureType::SimpleBarrier, "East Palisade Wall", 1800.f, "Settlement Palisade Barrier");

    addBuildPlot(-1250.f);
    addBuildPlot(1250.f);
}

void spawnNodes(SimulationRegistry& registry, float centerX, uint32_t) {
    for (int i = -1; i <= 1; ++i) {
        ResourceNode res;
        res.id = IDGenerator::generateEntityID();
        res.type = ResourceType::Food;
        res.worldX = centerX + (i * 2200.f) + (i >= 0 ? 2000.f : -2000.f);
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
        res.worldX = centerX + (i * 2600.f);
        res.worldY = 500.0f - 50.f;
        res.amount = 25;
        res.maxAmount = 25;
        res.regrowTimer = 0.f;
        registry.registerResource(res);
    }
    ResourceNode res;
    res.id = IDGenerator::generateEntityID();
    res.type = ResourceType::Stone;
    res.worldX = centerX + 3100.f;
    res.worldY = 500.0f - 50.f;
    res.amount = 20;
    res.maxAmount = 20;
    res.regrowTimer = 0.f;
    registry.registerResource(res);
}

EntityID PopulationGenerator::generatePlayerDynasty(SimulationRegistry& registry, uint32_t worldSeed) {
    std::vector<std::string> names = {"Caesar", "Maurice", "Cornelius", "Zira", "Aldo", "Blue Eyes", "Rocket", "Nova", "Luca"};
    std::vector<Trait> allTraits = {Trait::Brave, Trait::Strategic, Trait::Honorable, Trait::Charismatic, Trait::Hardworking, Trait::Clever};

    VillageData pVillage;
    pVillage.id = 1;
    pVillage.kingdomId = 1;
    pVillage.name = "Wintanceaster";
    pVillage.identity = VillageIdentity::Balanced;
    pVillage.homeChunkX = 0;
    pVillage.centerX = SettlementLayout::getPlayerCenterX();
    pVillage.centerY = 500.0f;
    pVillage.throneX = pVillage.centerX;
    pVillage.tier = SettlementTier::FirePit;
    pVillage.isGatheringActive = false;
    pVillage.amber = 12;
    pVillage.food = 65;
    pVillage.wood = 35;
    pVillage.stone = 15;
    pVillage.maxAmber = 60;
    pVillage.maxFood = 100;
    pVillage.maxWood = 80;
    pVillage.maxStone = 40;
    pVillage.toolsAxe = 0;
    pVillage.toolsPick = 0;
    pVillage.toolsSpear = 0;
    pVillage.toolsBasket = 1;
    pVillage.toolsTorch = 0;
    pVillage.availableAxeSlots = 0;
    pVillage.availableSpearSlots = 0;
    pVillage.availableBasketSlots = 1;

    float westWallX = pVillage.centerX - 1800.0f;
    float eastWallX = pVillage.centerX + 1800.0f;
    pVillage.borderMinX = westWallX - WALL_TO_BORDER_OFFSET;
    pVillage.borderMaxX = eastWallX + WALL_TO_BORDER_OFFSET;
    pVillage.territoryRadius = std::max(pVillage.borderMaxX - pVillage.centerX, pVillage.centerX - pVillage.borderMinX);

    spawnBaseStructures(registry, pVillage);

    DynastyData dyn;
    dyn.id = 1;
    dyn.name = "House of Wessex";
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
    founder.isMainApe = true;
    founder.amberCount = 8;
    founder.maxAmber = 40;
    founder.currentJob = Job::Idle;
    founder.currentKingdom = 1;

    ApeData spouse = createRandomApe(worldSeed + 1001, dyn.id, pVillage.id, names, allTraits, worldSeed);
    spouse.name = "Maya";
    spouse.age = 32.0f;
    spouse.gender = Gender::Female;
    spouse.worldX = pVillage.centerX + 100.0f;
    spouse.worldY = 500.0f;
    spouse.homeX = spouse.worldX;
    spouse.homeY = spouse.worldY;
    spouse.isMainApe = true;
    spouse.spouseId = founder.id;
    founder.spouseId = spouse.id;
    spouse.currentJob = Job::Idle;
    spouse.currentKingdom = 1;

    ApeData child = createRandomApe(worldSeed + 1002, dyn.id, pVillage.id, names, allTraits, worldSeed);
    child.name = "Tano";
    child.age = 15.0f;
    child.gender = Gender::Male;
    child.worldX = pVillage.centerX - 220.0f;
    child.worldY = 500.0f;
    child.homeX = child.worldX;
    child.homeY = child.worldY;
    child.isMainApe = true;
    child.fatherId = founder.id;
    child.motherId = spouse.id;
    child.currentJob = Job::Idle;
    child.currentKingdom = 1;

    ApeData sibling = createRandomApe(worldSeed + 1003, dyn.id, pVillage.id, names, allTraits, worldSeed);
    sibling.name = "Boro";
    sibling.age = 42.0f;
    sibling.gender = Gender::Male;
    sibling.worldX = pVillage.centerX + 1720.0f;
    sibling.worldY = 500.0f;
    sibling.homeX = sibling.worldX;
    sibling.homeY = sibling.worldY;
    sibling.isMainApe = true;
    sibling.currentJob = Job::Guard;
    sibling.currentKingdom = 1;

    ApeData worker1 = createRandomApe(worldSeed + 1004, 0, pVillage.id, names, allTraits, worldSeed);
    worker1.name = "Aldo";
    worker1.age = 28.0f;
    worker1.worldX = pVillage.centerX - 1720.0f;
    worker1.worldY = 500.0f;
    worker1.homeX = worker1.worldX;
    worker1.homeY = worker1.worldY;
    worker1.isMainApe = false;
    worker1.currentJob = Job::Guard;
    worker1.currentKingdom = 1;

    ApeData worker2 = createRandomApe(worldSeed + 1005, 0, pVillage.id, names, allTraits, worldSeed);
    worker2.name = "Maurice";
    worker2.age = 30.0f;
    worker2.worldX = pVillage.centerX - 350.0f;
    worker2.worldY = 500.0f;
    worker2.homeX = worker2.worldX;
    worker2.homeY = worker2.worldY;
    worker2.isMainApe = false;
    worker2.currentJob = Job::Idle;
    worker2.currentKingdom = 1;

    founder.children.push_back(child.id);
    spouse.children.push_back(child.id);
    founder.siblings.push_back(sibling.id);
    sibling.siblings.push_back(founder.id);

    dyn.founderId = founder.id;
    dyn.currentLeaderId = founder.id;
    dyn.primaryHeirId = child.id;
    dyn.members = {founder.id, spouse.id, child.id, sibling.id};

    pVillage.founderId = founder.id;
    pVillage.leaderId = founder.id;
    pVillage.members = {founder.id, spouse.id, child.id, sibling.id, worker1.id, worker2.id};

    registry.registerVillage(pVillage);
    registry.registerDynasty(dyn);
    registry.registerApe(founder);
    registry.registerApe(spouse);
    registry.registerApe(child);
    registry.registerApe(sibling);
    registry.registerApe(worker1);
    registry.registerApe(worker2);

    spawnNodes(registry, pVillage.centerX, worldSeed);

    KingdomData wessex;
    wessex.id = 1;
    wessex.name = "Wessex";
    wessex.leaderDynastyId = dyn.id;
    wessex.currentKingId = founder.id;
    wessex.capitalVillageId = pVillage.id;
    wessex.controlledVillages = {pVillage.id};
    wessex.color = sf::Color(145, 180, 75);
    wessex.territoryMinX = pVillage.borderMinX;
    wessex.territoryMaxX = pVillage.borderMaxX;
    wessex.permittedApes.insert(founder.id);
    registry.registerKingdom(wessex);

    return founder.id;
}

void PopulationGenerator::generateVillages(SimulationRegistry& registry, uint32_t worldSeed) {
    struct VillageBlueprint {
        VillageID vId;
        KingdomID kId;
        std::string kingdomName;
        std::string name;
        float worldX;
        sf::Color kingdomColor;
        bool isCapital;
    };

    std::vector<VillageBlueprint> route = {
        {10,  10, "Cornwallum",    "Kernow (Tintagel)", -18000.f, sf::Color(90, 160, 150), true},
        {20,  1,  "Wessex",        "Hamwic",             18000.f, sf::Color(145, 180, 75),  false},
        {30,  1,  "Wessex",        "Readingas",          36000.f, sf::Color(145, 180, 75),  false},
        {40,  1,  "Wessex",        "Lundenburh",         54000.f, sf::Color(145, 180, 75),  false},
        {50,  20, "East Anglia",   "Theodford",          72000.f, sf::Color(190, 110, 60),  true},
        {60,  30, "Mercia",        "Tamworthig",         90000.f, sf::Color(70, 120, 180),  true},
        {70,  30, "Mercia",        "Legaceaster",       108000.f, sf::Color(70, 120, 180),  false},
        {80,  40, "Five Boroughs", "Lindcylene",        126000.f, sf::Color(180, 60, 50),   true},
        {90,  50, "Jorvik",        "Jorvik",            144000.f, sf::Color(160, 45, 45),   true},
        {100, 60, "Northumbria",   "Dunholm",           162000.f, sf::Color(90, 100, 150),  true},
        {110, 60, "Northumbria",   "Bebbanburg",        180000.f, sf::Color(90, 100, 150),  false},
        {120, 70, "Alba",          "Dun Eideann",       198000.f, sf::Color(130, 90, 140),  true},
        {130, 70, "Alba",          "Sgain",             216000.f, sf::Color(130, 90, 140),  false}
    };

    std::vector<std::string> names = {"Spear", "Fang", "Goro", "Kala", "Kerchak", "Terk", "Ash", "Buck", "Brutus", "Goliath"};
    std::vector<Trait> allTraits = {Trait::Brave, Trait::Coward, Trait::Greedy, Trait::Cruel, Trait::Lazy, Trait::Strategic, Trait::Aggressive};

    EntityID playerApeId = registry.getControlledApe();

    for (const auto& bp : route) {
        VillageData village;
        village.id = bp.vId;
        village.kingdomId = bp.kId;
        village.name = bp.name;
        village.identity = VillageIdentity::Balanced;
        village.centerX = bp.worldX;
        village.homeChunkX = static_cast<int>(std::floor((village.centerX - 1000.f) / 2000.f));
        village.centerY = 500.0f;
        village.throneX = village.centerX;
        village.tier = SettlementTier::FirePit;
        village.isGatheringActive = false;
        village.amber = 10;
        village.food = 60;
        village.wood = 40;
        village.stone = 20;
        village.maxAmber = 60;
        village.maxFood = 100;
        village.maxWood = 80;
        village.maxStone = 40;
        village.toolsAxe = 0;
        village.toolsSpear = 1;
        village.availableAxeSlots = 0;
        village.availableSpearSlots = 0;
        village.availableBasketSlots = 1;

        float westWallX = village.centerX - 1800.0f;
        float eastWallX = village.centerX + 1800.0f;
        village.borderMinX = westWallX - WALL_TO_BORDER_OFFSET;
        village.borderMaxX = eastWallX + WALL_TO_BORDER_OFFSET;
        village.territoryRadius = std::max(village.borderMaxX - village.centerX, village.centerX - village.borderMinX);

        if (playerApeId != 0 && (bp.kId == 1 || bp.kId == 10)) {
            village.permittedApes.insert(playerApeId);
        }

        spawnBaseStructures(registry, village);

        int pop = 6;
        for (int i = 0; i < pop; ++i) {
            ApeData ape = createRandomApe(worldSeed + (static_cast<uint32_t>(bp.vId) * 1000) + i, 0, village.id, names, allTraits, worldSeed);
            ape.currentKingdom = bp.kId;

            float xOffset = 0.f;
            if (i == 0) {
                ape.currentJob = Job::Idle;
                xOffset = 0.f;
                village.founderId = ape.id;
                village.leaderId = ape.id;
            } else if (i == 1) {
                ape.currentJob = Job::Guard;
                xOffset = 1720.f;
            } else if (i == 2) {
                ape.currentJob = Job::Guard;
                xOffset = -1720.f;
            } else if (i == 3) {
                ape.currentJob = Job::Forage;
                xOffset = 700.f;
            } else {
                ape.currentJob = Job::Socialize;
                xOffset = (i % 2 == 0 ? 180.f : -180.f);
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

        KingdomData* kd = registry.getKingdom(bp.kId);
        if (!kd) {
            KingdomData newK;
            newK.id = bp.kId;
            newK.name = bp.kingdomName;
            newK.capitalVillageId = village.id;
            newK.currentKingId = village.leaderId;
            newK.controlledVillages.push_back(village.id);
            newK.territoryMinX = village.borderMinX;
            newK.territoryMaxX = village.borderMaxX;
            newK.color = bp.kingdomColor;
            if (playerApeId != 0 && (bp.kId == 1 || bp.kId == 10)) {
                newK.permittedApes.insert(playerApeId);
            }
            registry.registerKingdom(newK);
        } else {
            kd->controlledVillages.push_back(village.id);
            kd->territoryMinX = std::min(kd->territoryMinX, village.borderMinX);
            kd->territoryMaxX = std::max(kd->territoryMaxX, village.borderMaxX);
            if (bp.isCapital) {
                kd->capitalVillageId = village.id;
                kd->currentKingId = village.leaderId;
            }
        }
    }
}

}