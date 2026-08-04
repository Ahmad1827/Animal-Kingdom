#pragma once
#include "simulation/ApeData.h"
#include "simulation/VillageData.h"
#include "simulation/DynastyData.h"
#include "simulation/KingdomData.h"
#include "simulation/StructureData.h"
#include "simulation/ResourceNode.h"
#include "simulation/EventData.h"
#include "simulation/AnimalData.h"
#include "simulation/EntityID.h"
#include <unordered_map>
#include <vector>

namespace sim {

class SimulationRegistry {
private:
    std::unordered_map<EntityID, ApeData> apes;
    std::unordered_map<VillageID, VillageData> villages;
    std::unordered_map<DynastyID, DynastyData> dynasties;
    std::unordered_map<KingdomID, KingdomData> kingdoms;
    std::unordered_map<StructureID, StructureData> structures;
    std::unordered_map<EntityID, ResourceNode> resources;
    std::unordered_map<EventID, WorldEvent> activeEvents;
    std::unordered_map<EntityID, AnimalData> animals;
    std::vector<HistoricalRecord> history;
    
    Season currentSeason = Season::Spring;
    int currentYear = 1;
    int currentDay = 1;

public:
    void registerApe(const ApeData& ape) { apes[ape.id] = ape; }
    void registerVillage(const VillageData& village) { villages[village.id] = village; }
    void registerDynasty(const DynastyData& dynasty) { dynasties[dynasty.id] = dynasty; }
    void registerKingdom(const KingdomData& kingdom) { kingdoms[kingdom.id] = kingdom; }
    void registerStructure(const StructureData& structure) { structures[structure.id] = structure; }
    void registerResource(const ResourceNode& resource) { resources[resource.id] = resource; }
    void registerEvent(const WorldEvent& e) { activeEvents[e.id] = e; }
    void registerAnimal(const AnimalData& a) { animals[a.id] = a; }
    void addHistory(const HistoricalRecord& record) { history.push_back(record); }

    ApeData* getApe(EntityID id) { auto it = apes.find(id); return it != apes.end() ? &it->second : nullptr; }
    VillageData* getVillage(VillageID id) { auto it = villages.find(id); return it != villages.end() ? &it->second : nullptr; }
    DynastyData* getDynasty(DynastyID id) { auto it = dynasties.find(id); return it != dynasties.end() ? &it->second : nullptr; }
    KingdomData* getKingdom(KingdomID id) { auto it = kingdoms.find(id); return it != kingdoms.end() ? &it->second : nullptr; }
    StructureData* getStructure(StructureID id) { auto it = structures.find(id); return it != structures.end() ? &it->second : nullptr; }
    ResourceNode* getResource(EntityID id) { auto it = resources.find(id); return it != resources.end() ? &it->second : nullptr; }
    AnimalData* getAnimal(EntityID id) { auto it = animals.find(id); return it != animals.end() ? &it->second : nullptr; }

    std::unordered_map<EntityID, ApeData>& getAllApes() { return apes; }
    std::unordered_map<VillageID, VillageData>& getAllVillages() { return villages; }
    std::unordered_map<DynastyID, DynastyData>& getAllDynasties() { return dynasties; }
    std::unordered_map<KingdomID, KingdomData>& getAllKingdoms() { return kingdoms; }
    std::unordered_map<StructureID, StructureData>& getAllStructures() { return structures; }
    std::unordered_map<EntityID, ResourceNode>& getAllResources() { return resources; }
    std::unordered_map<EventID, WorldEvent>& getAllEvents() { return activeEvents; }
    std::unordered_map<EntityID, AnimalData>& getAllAnimals() { return animals; }
    const std::vector<HistoricalRecord>& getHistory() const { return history; }

    void setSeason(Season s) { currentSeason = s; }
    Season getSeason() const { return currentSeason; }
    void setDate(int year, int day) { currentYear = year; currentDay = day; }
    int getYear() const { return currentYear; }
    int getDay() const { return currentDay; }

    void removeEvent(EventID id) { activeEvents.erase(id); }
};

}