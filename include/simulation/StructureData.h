#pragma once
#include "simulation/EntityID.h"
#include <string>

namespace sim {
    enum class StructureType { Nest, StorageHut, WatchPlatform, WoodPile, StonePile, SimpleBarrier, TreeLadder, VillageCenter, BuilderHut, Bonfire };

    struct StructureData {
        StructureID id;
        StructureType type;
        VillageID villageId;
        std::string name;
        float worldX;
        float worldY;
        
        float progress;
        float maxProgress;
        EntityID currentBuilder;
        
        int reqWood;
        int reqStone;
        int curWood;
        int curStone;
        
        bool isPlanned;
        bool isUnderConstruction;
        bool isFinished;

        std::string benefitText;
    };
}