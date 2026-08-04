#pragma once
#include "simulation/EntityID.h"
#include "simulation/ResourceNode.h"
#include <string>
#include <vector>

namespace sim {

enum class Gender { Male, Female };
enum class Trait { Brave, Coward, Greedy, Honorable, Cruel, Charismatic, Lazy, Strategic, Impulsive, Curious, Energetic };
enum class Occupation { Unemployed, Ruler, Noble, Soldier, Hunter, Scout };
enum class Goal { None, Survive, Reproduce, AcquireWealth, UsurpThrone };
enum class Job { Idle, Sleep, Eat, Forage, CarryResource, ReturnHome, Guard, Wander, Socialize, Woodcutter, StoneGatherer, Builder, Craftsman };
enum class ToolType { None, StoneAxe, StonePick, WoodenSpear, Torch };

struct ApeData {
    EntityID id;
    DynastyID dynastyId;
    VillageID villageId;
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
    float homeX;
    float homeY;

    KingdomID currentKingdom;
    Occupation currentOccupation;
    Goal currentGoal;
    
    Job currentJob;
    EntityID currentTargetNode;
    StructureID currentTargetStructure;
    
    ResourceType carriedType;
    int carriedAmount;
    ToolType equippedTool;

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