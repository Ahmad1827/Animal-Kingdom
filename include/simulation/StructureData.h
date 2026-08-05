#pragma once
#include "simulation/EntityID.h"

namespace sim {
    enum class StructureType { Nest, StorageHut, WatchPlatform, WoodPile, StonePile, SimpleBarrier, TreeLadder, VillageCenter, BuilderHut, Bonfire };

    struct StructureData {
        StructureID id;
        StructureType type;
        VillageID villageId;
        float worldX;
        float worldY;
        
        float progress;
        float maxProgress;
        EntityID currentBuilder;
        
        int reqWood;
        int reqStone;
        int curWood;
        int curStone;
        
        bool isFinished;
    };
}