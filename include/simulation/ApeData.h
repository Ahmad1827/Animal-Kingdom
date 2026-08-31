#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "simulation/EntityID.h"
#include "simulation/ResourceNode.h"

namespace sim {

enum class Gender { 
    Male, 
    Female 
};

enum class Occupation { 
    Unemployed, 
    Woodcutter, 
    Gatherer, 
    Builder, 
    Guard, 
    Hunter 
};

enum class Goal { 
    Survive, 
    GatherFood, 
    GatherWood, 
    Build, 
    Socialize, 
    Sleep 
};

enum class Job { 
    Idle, 
    Woodcutter, 
    March, 
    Builder, 
    CarryResource, 
    Forage, 
    StoneGatherer, 
    Scout, 
    Patrol, 
    Guard, 
    Wander, 
    ReturnHome, 
    Sleep, 
    Socialize, 
    Eat, 
    Combat, 
    Intimidate, 
    Observe, 
    Muster, 
    Flee, 
    Gathering 
};

enum class ToolType { 
    None, 
    Basket, 
    StoneAxe, 
    StonePick, 
    WoodenSpear, 
    Torch 
};

enum class DiseaseType { 
    None, 
    Flu, 
    Fever, 
    Plague, 
    Parasite, 
    Infection 
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
    Impulsive, 
    Curious, 
    Energetic, 
    Clever, 
    Hardworking, 
    Patient, 
    Aggressive, 
    Perceptive 
};

enum class CouncilRole {
    None,
    WarChief,
    ChiefBuilder,
    LeadForager,
    Shaman
};

struct ApeSkills {
    float woodcutting = 1.0f;
    float gathering = 1.0f;
    float building = 1.0f;
    float combat = 1.0f;
    float scouting = 1.0f;
    float leadership = 1.0f;
};

struct ApeData {
    EntityID id = 0;
    DynastyID dynastyId = 0;
    VillageID villageId = 0;
    KingdomID currentKingdom = 0;
    std::string name = "Ape";

    bool isMainApe = false;
    CouncilRole councilRole = CouncilRole::None;

    float age = 20.0f;
    Gender gender = Gender::Male;
    float health = 100.0f;
    float hunger = 100.0f;
    bool alive = true;
    uint64_t birthDateTicks = 0;

    float worldX = 1000.0f;
    float worldY = 500.0f;
    int currentChunkX = 0;
    float homeX = 1000.0f;
    float homeY = 500.0f;

    Occupation currentOccupation = Occupation::Unemployed;
    Goal currentGoal = Goal::Survive;
    Job currentJob = Job::Idle;

    EntityID currentTargetNode = 0;
    EntityID currentTargetStructure = 0;
    EntityID currentCombatTarget = 0;
    EntityID currentArmyId = 0;

    bool hasTravelDestination = false;
    float travelDestinationX = 0.0f;

    ResourceType carriedType = ResourceType::None;
    int carriedAmount = 0;
    ToolType equippedTool = ToolType::None;

    int amberCount = 0;
    int maxAmber = 40;

    bool isCarryingBorder = false;

    DiseaseType currentDisease = DiseaseType::None;
    float diseaseSeverity = 0.0f;

    EntityID fatherId = 0;
    EntityID motherId = 0;
    EntityID spouseId = 0;
    std::vector<EntityID> children;
    std::vector<EntityID> siblings;
    std::vector<Trait> traits;
    ApeSkills skills;

    bool isWaitingForAudience = false;
    EntityID summonedRepId = 0;
    float meetingX = 0.0f;
    EntityID scheduledAudienceHost = 0;
};

}