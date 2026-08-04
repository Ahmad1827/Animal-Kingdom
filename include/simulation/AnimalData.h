#pragma once
#include "simulation/EntityID.h"

namespace sim {

enum class AnimalType { Jaguar, Snake, Eagle, Boar };
enum class AnimalState { Roaming, Hunting, Fleeing, Sleeping };

struct AnimalData {
    EntityID id;
    AnimalType type;
    float worldX;
    float worldY;
    float health;
    AnimalState state;
    EntityID targetId;
};

}