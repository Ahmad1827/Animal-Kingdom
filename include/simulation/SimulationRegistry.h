#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <algorithm>
#include <SFML/Graphics/Color.hpp>

#include "simulation/EntityID.h"
#include "simulation/ApeData.h"
#include "simulation/VillageData.h"
#include "simulation/StructureData.h"
#include "simulation/ResourceNode.h"
#include "dynasty/Character.h"
#include "dynasty/Clan.h"
#include "dynasty/Dynasty.h"
#include "dynasty/Faction.h"
#include "dynasty/Succession.h"

class WorldManager;

namespace sim {

enum class DiplomacyStatus {
    Neutral,
    Friendly,
    Alliance,
    Trade,
    Suspicious,
    Rival,
    War
};

enum class Season {
    Spring,
    Summer,
    Autumn,
    Winter
};

enum class ArmyObjective {
    Muster,
    March,
    Attack,
    Defend,
    Disband
};

enum class AnimalType {
    Boar,
    Tiger,
    Snake,
    Deer,
    Wolf
};

enum class AnimalState {
    Idle,
    Wander,
    Roaming,
    Hunting,
    Flee,
    Sleeping,
    Dead
};

struct AnimalData {
    EntityID id = 0;
    AnimalType type = AnimalType::Boar;
    AnimalState state = AnimalState::Idle;
    EntityID targetId = 0;
    float worldX = 0.0f;
    float worldY = 500.0f;
    float health = 100.0f;
};

enum class EventType {
    Drought,
    Flood,
    FruitBoom,
    Disease,
    ColdWave,
    Storm,
    HeavyRain,
    HeatWave,
    LocustSwarm,
    MeteorShower
};

struct WorldEvent {
    EventID id = 0;
    EventType type = EventType::Drought;
    uint64_t startTick = 0;
    uint64_t durationTicks = 0;
    float centerX = 0.0f;
    float radius = 500.0f;
    float intensity = 1.0f;
};

struct HistoricalRecord {
    int year = 1;
    int day = 1;
    std::string description;
};

struct DynastyData {
    DynastyID id = 0;
    std::string name = "Dynasty";
    int wealth = 100;
    int prestige = 50;
    int legitimacy = 100;
    EntityID founderId = 0;
    EntityID currentLeaderId = 0;
    EntityID primaryHeirId = 0;
    std::vector<EntityID> members;
};

struct ArmyData {
    EntityID id = 0;
    KingdomID homeKingdom = 0;
    KingdomID targetKingdom = 0;
    VillageID targetVillage = 0;
    EntityID leaderId = 0;
    float worldX = 0.0f;
    float targetX = 0.0f;
    int supplies = 0;
    std::vector<EntityID> members;
    ArmyObjective objective = ArmyObjective::Muster;
};

struct KingdomData {
    KingdomID id = 0;
    std::string name = "Kingdom";
    DynastyID leaderDynastyId = 0;
    EntityID currentKingId = 0;
    VillageID capitalVillageId = 0;
    std::vector<VillageID> controlledVillages;

    int population = 0;
    int militaryStrength = 10;
    int influence = 10;
    int totalResources = 0;

    int treasuryAmber = 0;
    int treasuryFood = 0;
    int treasuryWood = 0;
    int treasuryStone = 0;
    int treasuryTools = 0;

    float territoryMinX = 0.0f;
    float territoryMaxX = 0.0f;

    sf::Color color = sf::Color(180, 140, 50);
    std::unordered_set<KingdomID> knownKingdoms;
    std::unordered_set<EntityID> permittedApes;
    std::unordered_map<KingdomID, DiplomacyStatus> relations;
    std::unordered_map<KingdomID, float> borderTension;
    std::vector<EntityID> activeArmies;
};

class SimulationRegistry {
private:
    ::WorldManager* worldManager = nullptr;
    EntityID controlledApeId = 0;

    int currentYear = 1;
    int currentDay = 1;
    Season currentSeason = Season::Spring;

    std::unordered_map<EntityID, ApeData> apes;
    std::unordered_map<VillageID, VillageData> villages;
    std::unordered_map<KingdomID, KingdomData> kingdoms;
    std::unordered_map<DynastyID, DynastyData> dynasties;
    std::unordered_map<EntityID, StructureData> structures;
    std::unordered_map<EntityID, ResourceNode> resources;
    std::unordered_map<EntityID, ArmyData> armies;
    std::unordered_map<EntityID, AnimalData> animals;
    std::unordered_map<EventID, WorldEvent> events;
    std::vector<Faction> factions;
    std::unordered_map<EntityID, Character> characters;
    std::unordered_map<VillageID, Clan> clans;
    std::vector<HistoricalRecord> history;

public:
    void setWorldManager(::WorldManager* wm) { worldManager = wm; }
    ::WorldManager* getWorldManager() const { return worldManager; }

    EntityID generateEntityId() { return IDGenerator::generateEntityID(); }
    EntityID getControlledApe() const { return controlledApeId; }
    void setControlledApe(EntityID id) { controlledApeId = id; }

    int getYear() const { return currentYear; }
    int getDay() const { return currentDay; }
    Season getSeason() const { return currentSeason; }

    void setDate(int year, int day) { currentYear = year; currentDay = day; }
    void setSeason(Season s) { currentSeason = s; }

    void addHistory(const HistoricalRecord& rec) { history.push_back(rec); }
    const std::vector<HistoricalRecord>& getHistory() const { return history; }
    std::vector<HistoricalRecord>& getHistory() { return history; }

    void registerApe(const ApeData& ape) { apes[ape.id] = ape; }
    void registerVillage(const VillageData& v) { villages[v.id] = v; }
    void registerKingdom(const KingdomData& k) { kingdoms[k.id] = k; }
    void registerDynasty(const DynastyData& d) { dynasties[d.id] = d; }
    void registerStructure(const StructureData& s) { structures[s.id] = s; }
    void registerResource(const ResourceNode& r) { resources[r.id] = r; }
    void registerCharacter(const Character& c) { characters[c.id] = c; }
    void registerClan(const Clan& cl) { clans[cl.id] = cl; }
    void registerAnimal(const AnimalData& a) { animals[a.id] = a; }
    void registerArmy(const ArmyData& a) { armies[a.id] = a; }
    void registerEvent(const WorldEvent& e) { events[e.id] = e; }
    void registerFaction(const Faction& f) { factions.push_back(f); }
    void removeEvent(EventID id) { events.erase(id); }

    const std::unordered_map<EntityID, Character>& getAllCharacters() const { return characters; }
    std::unordered_map<EntityID, Character>& getAllCharacters() { return characters; }
    Clan* getClan(VillageID id) { auto it = clans.find(id); return it != clans.end() ? &it->second : nullptr; }

    ApeData* getApe(EntityID id) { auto it = apes.find(id); return it != apes.end() ? &it->second : nullptr; }
    VillageData* getVillage(VillageID id) { auto it = villages.find(id); return it != villages.end() ? &it->second : nullptr; }
    KingdomData* getKingdom(KingdomID id) { auto it = kingdoms.find(id); return it != kingdoms.end() ? &it->second : nullptr; }
    DynastyData* getDynasty(DynastyID id) { auto it = dynasties.find(id); return it != dynasties.end() ? &it->second : nullptr; }
    StructureData* getStructure(EntityID id) { auto it = structures.find(id); return it != structures.end() ? &it->second : nullptr; }
    ResourceNode* getResource(EntityID id) { auto it = resources.find(id); return it != resources.end() ? &it->second : nullptr; }
    ArmyData* getArmy(EntityID id) { auto it = armies.find(id); return it != armies.end() ? &it->second : nullptr; }
    AnimalData* getAnimal(EntityID id) { auto it = animals.find(id); return it != animals.end() ? &it->second : nullptr; }
    WorldEvent* getEvent(EventID id) { auto it = events.find(id); return it != events.end() ? &it->second : nullptr; }

    std::unordered_map<EntityID, ApeData>& getAllApes() { return apes; }
    std::unordered_map<VillageID, VillageData>& getAllVillages() { return villages; }
    std::unordered_map<KingdomID, KingdomData>& getAllKingdoms() { return kingdoms; }
    std::unordered_map<EntityID, StructureData>& getAllStructures() { return structures; }
    std::unordered_map<EntityID, ResourceNode>& getAllResources() { return resources; }
    std::unordered_map<EntityID, ArmyData>& getAllArmies() { return armies; }
    std::unordered_map<EntityID, AnimalData>& getAllAnimals() { return animals; }
    std::unordered_map<EventID, WorldEvent>& getAllEvents() { return events; }
    const std::unordered_map<EventID, WorldEvent>& getAllEvents() const { return events; }

    const std::vector<Faction>& getFactions() const { return factions; }
    std::vector<Faction>& getFactions() { return factions; }

    void updatePolitics(float, EntityID) {}
    void executeSuccession(DynastyID, EntityID) {}
};

}