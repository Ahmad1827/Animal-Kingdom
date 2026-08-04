#pragma once
#include "simulation/EntityID.h"
#include <string>
#include <vector>

namespace sim {
    struct VillageData {
        VillageID id;
        std::string name;
        EntityID founderId;
        EntityID leaderId;
        std::vector<EntityID> members;
        int homeChunkX;
        float centerX;
        float centerY;
        int food;
        int wood;
        int stone;
        float territoryRadius;
    };
}