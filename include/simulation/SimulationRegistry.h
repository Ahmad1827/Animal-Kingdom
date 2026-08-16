#pragma once
#include "simulation/ApeData.h"
#include "simulation/VillageData.h"
#include "simulation/DynastyData.h"
#include "simulation/KingdomData.h"
#include "simulation/ArmyData.h"
#include "simulation/StructureData.h"
#include "simulation/ResourceNode.h"
#include "simulation/EventData.h"
#include "simulation/AnimalData.h"
#include "simulation/EntityID.h"
#include "dynasty/Character.h"
#include "dynasty/Clan.h"
#include "dynasty/Faction.h"
#include "dynasty/Succession.h"
#include "dynasty/PoliticalSystem.h"
#include <unordered_map>
#include <vector>

class WorldManager;

namespace sim {

class SimulationRegistry {
private:
    std::unordered_map<EntityID, ApeData> apes;
    std::unordered_map<VillageID, VillageData> villages;
    std::unordered_map<DynastyID, DynastyData> dynasties;
    std::unordered_map<KingdomID, KingdomData> kingdoms;
    std::unordered_map<ArmyID, ArmyData> armies;
    std::unordered_map<StructureID, StructureData> structures;
    std::unordered_map<EntityID, ResourceNode> resources;
    std::unordered_map<EventID, WorldEvent> activeEvents;
    std::unordered_map<EntityID, AnimalData> animals;
    std::vector<HistoricalRecord> history;

    std::unordered_map<Character::ID, Character> characters;
    std::unordered_map<uint64_t, Clan> clans;
    std::vector<Faction> factions;
    WorldManager* worldManager = nullptr;

    Season currentSeason = Season::Spring;
    int currentYear = 1;
    int currentDay = 1;

public:
    void registerApe(const ApeData& ape) { apes[ape.id] = ape; }
    void registerVillage(const VillageData& village) { villages[village.id] = village; }
    void registerDynasty(const DynastyData& dynasty) { dynasties[dynasty.id] = dynasty; }
    void registerKingdom(const KingdomData& kingdom) { kingdoms[kingdom.id] = kingdom; }
    void registerArmy(const ArmyData& army) { armies[army.id] = army; }
    void registerStructure(const StructureData& structure) { structures[structure.id] = structure; }
    void registerResource(const ResourceNode& resource) { resources[resource.id] = resource; }
    void registerEvent(const WorldEvent& e) { activeEvents[e.id] = e; }
    void registerAnimal(const AnimalData& a) { animals[a.id] = a; }
    void addHistory(const HistoricalRecord& record) { history.push_back(record); }

    void registerCharacter(const Character& character) { characters[character.id] = character; }
    Character* getCharacter(Character::ID id) { auto it = characters.find(id); return it != characters.end() ? &it->second : nullptr; }
    std::unordered_map<Character::ID, Character>& getAllCharacters() { return characters; }
    const std::unordered_map<Character::ID, Character>& getAllCharacters() const { return characters; }

    void registerClan(const Clan& clan) { clans[clan.id] = clan; }
    Clan* getClan(uint64_t id) { auto it = clans.find(id); return it != clans.end() ? &it->second : nullptr; }
    std::unordered_map<uint64_t, Clan>& getAllClans() { return clans; }
    const std::unordered_map<uint64_t, Clan>& getAllClans() const { return clans; }

    std::vector<Faction>& getFactions() { return factions; }
    const std::vector<Faction>& getFactions() const { return factions; }
    void setWorldManager(WorldManager* wm) { worldManager = wm; }
    WorldManager* getWorldManager() const { return worldManager; }

    ApeData* getApe(EntityID id) { auto it = apes.find(id); return it != apes.end() ? &it->second : nullptr; }
    VillageData* getVillage(VillageID id) { auto it = villages.find(id); return it != villages.end() ? &it->second : nullptr; }
    DynastyData* getDynasty(DynastyID id) { auto it = dynasties.find(id); return it != dynasties.end() ? &it->second : nullptr; }
    KingdomData* getKingdom(KingdomID id) { auto it = kingdoms.find(id); return it != kingdoms.end() ? &it->second : nullptr; }
    ArmyData* getArmy(ArmyID id) { auto it = armies.find(id); return it != armies.end() ? &it->second : nullptr; }
    StructureData* getStructure(StructureID id) { auto it = structures.find(id); return it != structures.end() ? &it->second : nullptr; }
    ResourceNode* getResource(EntityID id) { auto it = resources.find(id); return it != resources.end() ? &it->second : nullptr; }
    AnimalData* getAnimal(EntityID id) { auto it = animals.find(id); return it != animals.end() ? &it->second : nullptr; }

    std::unordered_map<EntityID, ApeData>& getAllApes() { return apes; }
    std::unordered_map<VillageID, VillageData>& getAllVillages() { return villages; }
    std::unordered_map<DynastyID, DynastyData>& getAllDynasties() { return dynasties; }
    std::unordered_map<KingdomID, KingdomData>& getAllKingdoms() { return kingdoms; }
    std::unordered_map<ArmyID, ArmyData>& getAllArmies() { return armies; }
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
    void removeArmy(ArmyID id) { armies.erase(id); }

    void updatePolitics(float dt, EntityID alphaId) {
        for (auto& clanPair : clans) {
            DynastyData* dData = getDynasty(clanPair.second.dynastyId);
            if (!dData) continue;

            Dynasty dyn;
            dyn.id = dData->id;
            dyn.name = dData->name;
            dyn.currentAlphaId = alphaId;
            dyn.memberIds = dData->members;

            PoliticalSystem::updatePoliticalAI(clanPair.second, dyn, characters, factions, currentYear, currentDay);
        }
    }

    void executeSuccession(DynastyID dynId, EntityID currentAlphaId) {
        DynastyData* d = getDynasty(dynId);
        if (!d) return;

        SuccessionLaw law = SuccessionLaw::BLOODLINE_PRIMOGENITURE;
        for (const auto& pair : clans) {
            if (pair.second.dynastyId == dynId) {
                law = pair.second.successionLaw;
                break;
            }
        }

        Dynasty dynWrapper;
        dynWrapper.id = d->id;
        dynWrapper.name = d->name;
        dynWrapper.currentAlphaId = currentAlphaId;
        dynWrapper.memberIds = d->members;

        Character::ID heirId = SuccessionSystem::determineHeir(dynWrapper, characters, factions, law);
        if (heirId != Character::INVALID_ID) {
            if (characters.count(currentAlphaId)) {
                characters[currentAlphaId].isAlive = false;
                characters[currentAlphaId].logHistory(currentYear, currentDay, "Died of injuries / old age.");
            }
            if (characters.count(heirId)) {
                characters[heirId].logHistory(currentYear, currentDay, "Ascended as the new Alpha of " + d->name + ".");
            }

            for (auto& pair : clans) {
                if (pair.second.dynastyId == dynId) {
                    pair.second.adjustTension(20);
                }
            }
        }
    }
};

}