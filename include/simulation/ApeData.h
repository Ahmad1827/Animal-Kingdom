#pragma once
#include "simulation/EntityID.h"
#include "simulation/ResourceNode.h"
#include <string>
#include <vector>

namespace sim {

enum class Gender { Male, Female };
enum class Trait { Brave, Coward, Greedy, Honorable, Cruel, Charismatic, Lazy, Strategic, Impulsive, Curious, Energetic };
enum class Occupation { Unemployed, Ruler, Noble, Soldier, Hunter, Scout, Crafter };
enum class Goal { None, Survive, Reproduce, AcquireWealth, UsurpThrone };
enum class Job { Idle, Sleep, Eat, Forage, CarryResource, ReturnHome, Guard, Wander, Socialize, Woodcutter, StoneGatherer, Builder, Craftsman, Scout, Patrol, Muster, March, Combat, Observe, Intimidate, Flee };
enum class ToolType { None, StoneAxe, StonePick, WoodenSpear, Torch, Basket, Rope };
enum class DiseaseType { None, Fever, Parasites, Infection };

struct SkillSet {
    float woodcutting = 1.0f;
    float gathering = 1.0f;
    float building = 1.0f;
    float crafting = 1.0f;
    float scouting = 1.0f;
    float leadership = 1.0f;
    float combat = 1.0f;
};

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
    SkillSet skills;
    
    float worldX;
    float worldY;
    int currentChunkX;
    int currentChunkY;
    int homeChunkX;
    float homeX;
    float homeY;

    KingdomID currentKingdom = 0;
    ArmyID currentArmyId = 0;
    Occupation currentOccupation = Occupation::Unemployed;
    Goal currentGoal = Goal::None;
    
    Job currentJob = Job::Idle;
    EntityID currentTargetNode = 0;
    StructureID currentTargetStructure = 0;
    VillageID currentTargetVillage = 0;
    EntityID currentCombatTarget = 0;
    
    ResourceType carriedType = ResourceType::None;
    int carriedAmount = 0;
    ToolType equippedTool = ToolType::None;
    DiseaseType currentDisease = DiseaseType::None;
    float diseaseSeverity = 0.0f;

    EntityID fatherId = 0;
    EntityID motherId = 0;
    EntityID spouseId = 0;
    std::vector<EntityID> children;
    std::vector<EntityID> siblings;
    std::vector<EntityID> relationships;

    bool alive = true;
    uint64_t birthDateTicks = 0;

    bool hasTravelDestination = false;
    float travelDestinationX = 0.0f;
    
    bool isWaitingForAudience = false;
    EntityID summonedRepId = 0;
    float meetingX = 0.0f;
};

}