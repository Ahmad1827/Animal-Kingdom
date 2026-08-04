#pragma once
#include "simulation/EntityID.h"
#include "simulation/StructureData.h"
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
        float territoryRadius;

        int food;
        int wood;
        int stone;
        
        int toolsAxe;
        int toolsPick;
        int toolsSpear;
        int toolsTorch;

        std::vector<StructureID> finishedStructures;
        std::vector<StructureID> constructionQueue;
    };
}