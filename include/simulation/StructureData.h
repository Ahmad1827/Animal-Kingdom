#pragma once
#include <string>
#include "simulation/EntityID.h"

namespace sim {

enum class StructureType {
    VillageCenter,
    Bonfire,
    WoodPile,
    StonePile,
    SimpleBarrier,
    StorageHut,
    Nest,
    BuilderHut,
    WatchPlatform,
    Granary,
    ToolRack,
    Barricade,
    Watchtower,
    Armory,
    EmptyPlot,
    TreeLadder,
    Throne
};

struct StructureData {
    StructureID id = 0;
    StructureType type = StructureType::VillageCenter;
    std::string name;
    VillageID villageId = 0;
    float worldX = 0.0f;
    float worldY = 500.0f;
    float progress = 0.0f;
    float maxProgress = 100.0f;
    int reqWood = 0;
    int reqStone = 0;
    int curWood = 0;
    int curStone = 0;

    int tier = 1;
    int requiredAmber = 0;
    int requiredWood = 0;
    int requiredStone = 0;

    int axeCount = 0;
    int claimedAxes = 0;

    bool isPlanned = false;
    bool isUnderConstruction = false;
    bool isFinished = true;
    std::string benefitText;
    EntityID currentBuilder = 0;
};

}