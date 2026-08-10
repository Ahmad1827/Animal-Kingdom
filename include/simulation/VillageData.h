#pragma once
#include "simulation/EntityID.h"
#include "simulation/StructureData.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace sim {

enum class VillageIdentity { Balanced, WoodFocused, StoneFocused, FoodRich, Aggressive, Peaceful, Expansionist };
enum class Reputation { Unknown, Friendly, Neutral, Suspicious, Hostile };

struct TradeMission {
    VillageID targetVillage;
    ResourceType resourceGiven;
    int amountGiven;
    ResourceType resourceSought;
};

struct VillageData {
    VillageID id;
    KingdomID kingdomId = 0;
    std::string name;
    VillageIdentity identity;
    EntityID founderId;
    EntityID leaderId;
    std::vector<EntityID> members;
    
    int homeChunkX;
    float centerX;
    float centerY;
    float territoryRadius = 1500.0f;

    float borderMinX = 0.f;
    float borderMaxX = 0.f;
    int food = 0;
    int wood = 0;
    int stone = 0;
    
    int toolsAxe = 0;
    int toolsPick = 0;
    int toolsSpear = 0;
    int toolsTorch = 0;
    int toolsBasket = 0;
    int toolsRope = 0;

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