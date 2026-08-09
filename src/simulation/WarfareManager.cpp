#include "simulation/WarfareManager.h"
#include "simulation/EntityID.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace sim {

void WarfareManager::update(SimulationRegistry& registry, uint64_t ticks) {
    if (ticks % 300 == 0) {
        updateDiplomaticTension(registry, ticks);
        processArmies(registry);
    }
}

void WarfareManager::declareWar(SimulationRegistry& registry, KingdomID initiator, KingdomID target, const std::string& reason) {
    KingdomData* k1 = registry.getKingdom(initiator);
    KingdomData* k2 = registry.getKingdom(target);
    
    if (k1 && k2) {
        k1->relations[target] = DiplomacyStatus::War;
        k2->relations[initiator] = DiplomacyStatus::War;
        
        HistoricalRecord rec;
        rec.year = registry.getYear();
        rec.day = registry.getDay();
        rec.description = k1->name + " declared war on " + k2->name + ". Reason: " + reason;
        registry.addHistory(rec);
    }
}

void WarfareManager::cancelWar(SimulationRegistry& registry, KingdomID k1, KingdomID k2) {
    KingdomData* kd1 = registry.getKingdom(k1);
    KingdomData* kd2 = registry.getKingdom(k2);
    
    if (kd1 && kd2) {
        kd1->relations[k2] = DiplomacyStatus::Neutral;
        kd2->relations[k1] = DiplomacyStatus::Neutral;
        kd1->borderTension[k2] = 0.0f;
        kd2->borderTension[k1] = 0.0f;
        
        HistoricalRecord rec;
        rec.year = registry.getYear();
        rec.day = registry.getDay();
        rec.description = kd1->name + " and " + kd2->name + " signed a peace treaty.";
        registry.addHistory(rec);
    }
}

ArmyID WarfareManager::issueMusterOrder(SimulationRegistry& registry, KingdomID kingdomId, KingdomID targetKingdom, EntityID leaderId) {
    KingdomData* kd = registry.getKingdom(kingdomId);
    if (!kd) return 0;

    ArmyData army;
    army.id = IDGenerator::generateStructureID();
    army.homeKingdom = kingdomId;
    army.targetKingdom = targetKingdom;
    army.leaderId = leaderId;
    army.objective = ArmyObjective::Muster;
    
    VillageData* capital = registry.getVillage(kd->capitalVillageId);
    if (capital) {
        army.worldX = capital->centerX;
    }
    
    if (kd->treasuryFood >= 50) {
        kd->treasuryFood -= 50;
        army.supplies = 50;
    }
    
    for (VillageID vid : kd->controlledVillages) {
        VillageData* v = registry.getVillage(vid);
        if (v) {
            int drafted = 0;
            for (EntityID memberId : v->members) {
                if (drafted >= 3) break;
                ApeData* ape = registry.getApe(memberId);
                if (ape && ape->alive && ape->equippedTool == ToolType::WoodenSpear && ape->currentArmyId == 0) {
                    ape->currentArmyId = army.id;
                    ape->currentJob = Job::Muster;
                    army.members.push_back(ape->id);
                    drafted++;
                }
            }
        }
    }
    
    if (army.members.empty()) return 0;
    
    kd->activeArmies.push_back(army.id);
    registry.registerArmy(army);
    
    HistoricalRecord rec;
    rec.year = registry.getYear();
    rec.day = registry.getDay();
    rec.description = kd->name + " raised an army.";
    registry.addHistory(rec);
    
    return army.id;
}

void WarfareManager::updateDiplomaticTension(SimulationRegistry& registry, uint64_t ticks) {
    auto& kingdoms = registry.getAllKingdoms();
    for (auto it1 = kingdoms.begin(); it1 != kingdoms.end(); ++it1) {
        for (auto it2 = std::next(it1); it2 != kingdoms.end(); ++it2) {
            KingdomData& k1 = it1->second;
            KingdomData& k2 = it2->second;
            
            if (k1.knownKingdoms.find(k2.id) != k1.knownKingdoms.end()) {
                bool sharedBorder = (k1.territoryMaxX >= k2.territoryMinX && k1.territoryMinX <= k2.territoryMaxX);
                
                if (sharedBorder) {
                    k1.borderTension[k2.id] += 1.0f;
                    k2.borderTension[k1.id] += 1.0f;
                    
                    if (k1.relations[k2.id] == DiplomacyStatus::Neutral && k1.borderTension[k2.id] > 50.0f) {
                        k1.relations[k2.id] = DiplomacyStatus::Rival;
                        k2.relations[k1.id] = DiplomacyStatus::Rival;
                    }
                    
                    if (k1.relations[k2.id] == DiplomacyStatus::Rival && k1.borderTension[k2.id] > 100.0f) {
                        if (std::rand() % 100 < 5) {
                            declareWar(registry, k1.id, k2.id, "Border dispute and expansion pressure.");
                        }
                    }
                } else {
                    k1.borderTension[k2.id] = std::max(0.0f, k1.borderTension[k2.id] - 0.5f);
                    k2.borderTension[k1.id] = std::max(0.0f, k2.borderTension[k1.id] - 0.5f);
                }
            }
        }
    }
}

void WarfareManager::processArmies(SimulationRegistry& registry) {
    auto& armies = registry.getAllArmies();
    for (auto& pair : armies) {
        ArmyData& army = pair.second;
        
        if (army.objective == ArmyObjective::Muster) {
            bool allArrived = true;
            for (EntityID id : army.members) {
                ApeData* ape = registry.getApe(id);
                if (ape && ape->alive) {
                    if (std::abs(ape->worldX - army.worldX) > 200.0f) {
                        allArrived = false;
                        break;
                    }
                }
            }
            if (allArrived && !army.members.empty()) {
                army.objective = ArmyObjective::March;
                
                // Target Resolution Fork
                if (army.targetKingdom != 0) {
                    KingdomData* target = registry.getKingdom(army.targetKingdom);
                    if (target) {
                        VillageData* targetCap = registry.getVillage(target->capitalVillageId);
                        if (targetCap) army.targetX = targetCap->centerX;
                    }
                } else if (army.targetVillage != 0) {
                    VillageData* targetV = registry.getVillage(army.targetVillage);
                    if (targetV) army.targetX = targetV->centerX;
                }
            }
        } else if (army.objective == ArmyObjective::March) {
            army.worldX += (army.targetX > army.worldX) ? 10.0f : -10.0f;
            if (std::abs(army.worldX - army.targetX) < 300.0f) {
                army.objective = ArmyObjective::Attack;
            }
        }
    }
}

ArmyID WarfareManager::declareRaid(SimulationRegistry& registry, KingdomID initiator, VillageID targetVillage, EntityID leaderId, const std::string& reason) {
    KingdomData* kd = registry.getKingdom(initiator);
    VillageData* vTarget = registry.getVillage(targetVillage);
    
    // Validate initiator, target, and ensure target is strictly an independent village
    if (!kd || !vTarget || vTarget->kingdomId != 0) return 0;
    if (kd->capitalVillageId != 0) {
        vTarget->relations[kd->capitalVillageId] = sim::Reputation::Hostile;
    }
    ArmyData army;
    army.id = IDGenerator::generateStructureID();
    army.homeKingdom = initiator;
    army.targetKingdom = 0;          // Explicitly clear kingdom target
    army.targetVillage = targetVillage; // Assign village target
    army.leaderId = leaderId;
    army.objective = ArmyObjective::Muster;
    
    VillageData* capital = registry.getVillage(kd->capitalVillageId);
    if (capital) {
        army.worldX = capital->centerX;
    }
    
    // Resource cost for raising an army
    if (kd->treasuryFood >= 50) {
        kd->treasuryFood -= 50;
        army.supplies = 50;
    }
    
    // Draft members exactly as issueMusterOrder does
    for (VillageID vid : kd->controlledVillages) {
        VillageData* v = registry.getVillage(vid);
        if (v) {
            int drafted = 0;
            for (EntityID memberId : v->members) {
                if (drafted >= 3) break;
                ApeData* ape = registry.getApe(memberId);
                if (ape && ape->alive && ape->equippedTool == ToolType::WoodenSpear && ape->currentArmyId == 0) {
                    ape->currentArmyId = army.id;
                    ape->currentJob = Job::Muster;
                    army.members.push_back(ape->id);
                    drafted++;
                }
            }
        }
    }
    
    if (army.members.empty()) return 0;
    
    kd->activeArmies.push_back(army.id);
    registry.registerArmy(army);
    
    HistoricalRecord rec;
    rec.year = registry.getYear();
    rec.day = registry.getDay();
    rec.description = kd->name + " initiated a raid against the independent village of " + vTarget->name + ". Reason: " + reason;
    registry.addHistory(rec);
    
    return army.id;
}

}