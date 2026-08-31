#pragma once
#include "simulation/EntityID.h"
#include "simulation/StructureData.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace sim {

enum class VillageIdentity { 
    Balanced, 
    WoodFocused, 
    StoneFocused, 
    FoodRich, 
    Aggressive, 
    Peaceful, 
    Expansionist 
};

enum class SettlementTier { 
    FirePit, 
    Camp, 
    Village, 
    Stronghold 
};

enum class Reputation { 
    Unknown, 
    Friendly, 
    Neutral, 
    Suspicious, 
    Hostile 
};

struct TradeMission {
    VillageID targetVillage = 0;
    ResourceType resourceGiven = ResourceType::None;
    int amountGiven = 0;
    ResourceType resourceSought = ResourceType::None;
};

struct VillageData {
    VillageID id = 0;
    KingdomID kingdomId = 0;
    std::string name;
    VillageIdentity identity = VillageIdentity::Balanced;
    SettlementTier tier = SettlementTier::FirePit;
    EntityID founderId = 0;
    EntityID leaderId = 0;
    std::vector<EntityID> members;
    
    int homeChunkX = 0;
    float centerX = 1000.0f;
    float centerY = 500.0f;
    float territoryRadius = 2200.0f;

    float borderMinX = -1200.0f;
    float borderMaxX = 3200.0f;

    int food = 0;
    int wood = 0;
    int stone = 0;
    int amber = 0;

    int maxFood = 100;
    int maxWood = 100;
    int maxStone = 100;
    int maxAmber = 100;
    
    int toolsAxe = 0;
    int toolsPick = 0;
    int toolsSpear = 0;
    int toolsTorch = 0;
    int toolsBasket = 0;
    int toolsRope = 0;

    int availableAxeSlots = 0;
    int availableSpearSlots = 0;
    int availableBasketSlots = 0;

    bool isExpandingBorder = false;
    bool expandingSideRight = true;
    float targetBorderX = 0.0f;
    EntityID borderMoverApe = 0;
    EntityID borderStructureId = 0;

    bool isGatheringActive = false;
    float throneX = 1000.0f;

    EntityID warChiefId = 0;
    EntityID chiefBuilderId = 0;
    EntityID leadForagerId = 0;
    EntityID shamanId = 0;

    bool isMigrating = false;
    float migrationTargetX = 0.0f;

    std::vector<StructureID> finishedStructures;
    std::vector<StructureID> constructionQueue;

    std::unordered_set<VillageID> knownVillages;
    std::unordered_map<VillageID, Reputation> relations;
    std::unordered_set<EntityID> knownResources;
    std::vector<TradeMission> activeTrades;
    std::vector<std::string> villageMemory;
    std::unordered_map<sim::EntityID, int> personalOpinions;
    std::unordered_set<sim::EntityID> permittedApes;
};

}