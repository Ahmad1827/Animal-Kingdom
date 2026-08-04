#include "simulation/KingdomManager.h"
#include "simulation/EntityID.h"
#include <algorithm>
#include <cmath>

namespace sim {

void KingdomManager::update(SimulationRegistry& registry, uint64_t ticks) {
    if (ticks % 1440 != 0) return;

    for (auto& pair : registry.getAllKingdoms()) {
        KingdomData& kingdom = pair.second;
        collectTribute(registry, kingdom);
        updateInfluence(registry, kingdom);
        updateDiplomacy(registry, kingdom, ticks);
    }
}

void KingdomManager::collectTribute(SimulationRegistry& registry, KingdomData& kingdom) {
    for (VillageID vid : kingdom.controlledVillages) {
        VillageData* v = registry.getVillage(vid);
        if (v && v->id != kingdom.capitalVillageId) {
            int foodTax = static_cast<int>(v->food * 0.1f);
            int woodTax = static_cast<int>(v->wood * 0.1f);
            int stoneTax = static_cast<int>(v->stone * 0.1f);
            
            v->food -= foodTax;
            v->wood -= woodTax;
            v->stone -= stoneTax;
            
            kingdom.treasuryFood += foodTax;
            kingdom.treasuryWood += woodTax;
            kingdom.treasuryStone += stoneTax;
        }
    }
}

void KingdomManager::updateInfluence(SimulationRegistry& registry, KingdomData& kingdom) {
    int totalPop = 0;
    int totalStructs = 0;
    
    for (VillageID vid : kingdom.controlledVillages) {
        VillageData* v = registry.getVillage(vid);
        if (v) {
            totalPop += v->members.size();
            totalStructs += v->finishedStructures.size();
        }
    }
    
    kingdom.population = totalPop;
    kingdom.totalResources = kingdom.treasuryFood + kingdom.treasuryWood + kingdom.treasuryStone + kingdom.treasuryTools;
    
    float dynPrestige = 0.0f;
    DynastyData* d = registry.getDynasty(kingdom.leaderDynastyId);
    if (d) dynPrestige = static_cast<float>(d->prestige);

    kingdom.influence = (totalPop * 2.0f) + (totalStructs * 5.0f) + (dynPrestige * 10.0f) + (kingdom.controlledVillages.size() * 50.0f);
}

void KingdomManager::updateDiplomacy(SimulationRegistry& registry, KingdomData& kingdom, uint64_t ticks) {
    for (KingdomID otherId : kingdom.knownKingdoms) {
        if (kingdom.relations.find(otherId) == kingdom.relations.end()) {
            kingdom.relations[otherId] = DiplomacyStatus::Neutral;
        }
        
        KingdomData* other = registry.getKingdom(otherId);
        if (other && kingdom.relations[otherId] == DiplomacyStatus::Neutral) {
            if (std::abs(kingdom.influence - other->influence) < 50.0f && ticks % 14400 == 0) {
                kingdom.relations[otherId] = DiplomacyStatus::Trade;
                other->relations[kingdom.id] = DiplomacyStatus::Trade;
                
                HistoricalRecord rec;
                rec.year = registry.getYear();
                rec.day = registry.getDay();
                rec.description = kingdom.name + " signed a trade agreement with " + other->name + ".";
                registry.addHistory(rec);
            }
        }
    }
}

void KingdomManager::handleFirstContact(SimulationRegistry& registry, KingdomID k1, KingdomID k2) {
    KingdomData* kd1 = registry.getKingdom(k1);
    KingdomData* kd2 = registry.getKingdom(k2);
    
    if (kd1 && kd2) {
        if (kd1->knownKingdoms.find(k2) == kd1->knownKingdoms.end()) {
            kd1->knownKingdoms.insert(k2);
            kd2->knownKingdoms.insert(k1);
            
            kd1->relations[k2] = DiplomacyStatus::Neutral;
            kd2->relations[k1] = DiplomacyStatus::Neutral;

            HistoricalRecord rec;
            rec.year = registry.getYear();
            rec.day = registry.getDay();
            rec.description = kd1->name + " encountered " + kd2->name + ".";
            registry.addHistory(rec);
            
            DynastyData* d1 = registry.getDynasty(kd1->leaderDynastyId);
            DynastyData* d2 = registry.getDynasty(kd2->leaderDynastyId);
            if (d1) d1->prestige += 10;
            if (d2) d2->prestige += 10;
        }
    }
}

void KingdomManager::spawnDebugKingdom(SimulationRegistry& registry, VillageID capitalId, DynastyID dynastyId, EntityID kingId) {
    KingdomData k;
    k.id = IDGenerator::generateStructureID();
    k.name = "Kingdom of " + std::to_string(k.id % 1000);
    k.color = sf::Color(std::rand() % 255, std::rand() % 255, std::rand() % 255);
    k.capitalVillageId = capitalId;
    k.leaderDynastyId = dynastyId;
    k.currentKingId = kingId;
    k.controlledVillages.push_back(capitalId);
    
    VillageData* v = registry.getVillage(capitalId);
    if (v) v->kingdomId = k.id;
    
    ApeData* a = registry.getApe(kingId);
    if (a) a->currentKingdom = k.id;

    registry.registerKingdom(k);
    
    HistoricalRecord rec;
    rec.year = registry.getYear();
    rec.day = registry.getDay();
    rec.description = k.name + " was founded.";
    registry.addHistory(rec);
}

}