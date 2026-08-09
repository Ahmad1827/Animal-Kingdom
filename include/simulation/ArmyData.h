#pragma once
#include "simulation/EntityID.h"
#include <vector>

namespace sim {

enum class ArmyObjective { Muster, March, Defend, Attack, Disband };

struct ArmyData {
    ArmyID id;
    KingdomID homeKingdom = 0;
    KingdomID targetKingdom = 0;
    VillageID targetVillage = 0;
    EntityID leaderId = 0;
    std::vector<EntityID> members;
    
    float morale = 100.0f;
    int supplies = 0;
    ArmyObjective objective = ArmyObjective::Muster;
    
    float worldX = 0.0f;
    float targetX = 0.0f;
};

}