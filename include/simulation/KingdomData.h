#pragma once
#include "simulation/EntityID.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace sim {

enum class DiplomacyStatus { Unknown, Neutral, Peace, Friendly, Alliance, Trade, Rival, War };

struct KingdomData {
    KingdomID id;
    std::string name;
    sf::Color color;
    VillageID capitalVillageId = 0;
    DynastyID leaderDynastyId = 0;
    EntityID currentKingId = 0;
    
    int population = 0;
    int totalResources = 0;
    int militaryStrength = 0;
    
    int treasuryFood = 0;
    int treasuryWood = 0;
    int treasuryStone = 0;
    int treasuryTools = 0;
    
    float influence = 0.0f;
    float happiness = 100.0f;
    
    float territoryMinX = 0.0f;
    float territoryMaxX = 0.0f;
    
    std::vector<VillageID> controlledVillages;
    std::unordered_set<KingdomID> knownKingdoms;
    std::unordered_map<KingdomID, DiplomacyStatus> relations;
    std::unordered_map<KingdomID, float> borderTension;
    std::unordered_set<sim::EntityID> permittedApes;
    std::vector<ArmyID> activeArmies;
};

}