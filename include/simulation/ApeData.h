#pragma once
#include "simulation/EntityID.h"
#include <string>
#include <vector>

namespace sim {

enum class Gender { Male, Female };
enum class Trait { Brave, Coward, Greedy, Honorable, Cruel, Charismatic, Lazy, Strategic, Impulsive, Curious, Energetic };
enum class Occupation { Unemployed, Ruler, Noble, Soldier, Hunter, Scout };
enum class Goal { None, Survive, Reproduce, AcquireWealth, UsurpThrone };

struct ApeData {
    EntityID id;
    DynastyID dynastyId;
    std::string name;
    float age;
    Gender gender;
    float health;
    float hunger;
    std::vector<Trait> traits;
    
    float worldX;
    float worldY;
    int currentChunkX;
    int currentChunkY;
    int homeChunkX;

    KingdomID currentKingdom;
    Occupation currentOccupation;
    Goal currentGoal;

    EntityID fatherId = 0;
    EntityID motherId = 0;
    EntityID spouseId = 0;
    std::vector<EntityID> children;
    std::vector<EntityID> siblings;
    std::vector<EntityID> relationships;

    bool alive;
    uint64_t birthDateTicks;
};

}