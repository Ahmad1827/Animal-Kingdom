#pragma once
#include "simulation/EntityID.h"
#include <string>
#include <vector>
#include <utility>

namespace sim {

struct KingdomData {
    KingdomID id;
    std::string name;
    EntityID leaderId;
    int capitalChunkX;
    int capitalChunkY;
    std::vector<std::pair<int, int>> villages;
    int population;
    int armySize;
    int food;
    int wood;
    int stone;
    int gold;
    int technologyLevel;
    std::vector<KingdomID> allies;
    std::vector<KingdomID> enemies;
};

}