#pragma once
#include "simulation/EntityID.h"
#include "simulation/ResourceNode.h"
#include <string>
#include <vector>

namespace sim {

enum class Gender { Male, Female };
enum class Occupation { Unemployed, Forager, Woodcutter, Hunter, Builder, Guard, Scout, Craftsman };
enum class Goal { Survive, GatherFood, GatherWood, Build, ProtectVillage, Socialize, Sleep };

enum class Trait { Brave, Coward, Greedy, Honorable, Cruel, Charismatic, Lazy, Strategic, Impulsive, Curious, Energetic };

enum class Job { Idle, Sleep, Eat, Forage, CarryResource, ReturnHome, Guard, Wander, Socialize, Woodcutter, StoneGatherer, Builder, Craftsman, Scout, Patrol, Muster, March, Combat, Observe, Intimidate, Flee };

enum class ToolType { None, StoneAxe, StonePick, WoodenSpear, Torch, Basket, Rope };

enum class DiseaseType { None, Fever, Rabies, Parasite, Plague };

struct ApeSkills {
    float foraging = 1.0f;
    float woodcutting = 1.0f;
    float building = 1.0f;
    float combat = 1.0f;
    float leadership = 1.0f;
    float gathering = 1.0f;
    float scouting = 1.0f;
};

struct ApeData {
    EntityID id = 0;
    std::string name = "Ape";
    float age = 20.0f;
    Gender gender = Gender::Male;
    float health = 100.0f;
    float hunger = 100.0f;
    bool alive = true;
    uint64_t birthDateTicks = 0;

    DynastyID dynastyId = 0;
    VillageID villageId = 0;
    KingdomID currentKingdom = 0;

    Occupation currentOccupation = Occupation::Unemployed;
    Goal currentGoal = Goal::Survive;
    Job currentJob = Job::Idle;

    float worldX = 1000.0f;
    float worldY = 500.0f;
    float homeX = 1000.0f;
    float homeY = 500.0f;
    int currentChunkX = 0;

    EntityID spouseId = 0;
    EntityID fatherId = 0;
    EntityID motherId = 0;
    std::vector<EntityID> children;
    std::vector<EntityID> siblings;

    std::vector<Trait> traits;
    ApeSkills skills;

    ToolType equippedTool = ToolType::None;
    ResourceType carriedType = ResourceType::None;
    int carriedAmount = 0;

    int amberCount = 0;
    int maxAmber = 50;

    EntityID currentTargetNode = 0;
    EntityID currentTargetStructure = 0;
    EntityID currentArmyId = 0;
    EntityID currentCombatTarget = 0;

    DiseaseType currentDisease = DiseaseType::None;
    float diseaseSeverity = 0.0f;

    bool hasTravelDestination = false;
    float travelDestinationX = 0.0f;

    bool isWaitingForAudience = false;
    EntityID summonedRepId = 0;
    EntityID scheduledAudienceHost = 0;
    float meetingX = 0.0f;
};

}