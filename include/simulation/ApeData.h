#pragma once
#include "simulation/EntityID.h"
#include <string>
#include <vector>

namespace sim {

enum class Gender {
    Male,
    Female
};

enum class Trait {
    Brave,
    Coward,
    Greedy,
    Honorable,
    Cruel,
    Charismatic,
    Lazy,
    Strategic,
    Impulsive
};

enum class Occupation {
    Unemployed,
    Ruler,
    Noble,
    Soldier,
    Hunter,
    Scout
};

enum class Goal {
    None,
    Survive,
    Reproduce,
    AcquireWealth,
    UsurpThrone
};

struct ApeData {
    EntityID id;
    DynastyID dynastyId;
    std::string name;
    float age;
    Gender gender;
    float health;
    float hunger;
    std::vector<Trait> traits;
    int currentChunkX;
    int currentChunkY;
    KingdomID currentKingdom;
    Occupation currentOccupation;
    Goal currentGoal;
    std::vector<EntityID> relationships;
    bool alive;
    uint64_t birthDateTicks;
};

}