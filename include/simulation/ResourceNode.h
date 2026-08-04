#pragma once
#include "simulation/EntityID.h"

namespace sim {
    enum class ResourceType { None, Food, Wood, Stone };

    struct ResourceNode {
        EntityID id;
        ResourceType type;
        float worldX;
        float worldY;
        int amount;
        int maxAmount;
        float regrowTimer;
    };
}