#include "simulation/WarfareManager.h"
#include <algorithm>
#include <cmath>

namespace sim {

void WarfareManager::update(SimulationRegistry& registry, float dt) {
    updateWarProgression(registry, dt);
    processArmies(registry);
}

uint32_t WarfareManager::declareWarWithGoal(SimulationRegistry& registry, KingdomID initiator, KingdomID target, VillageID targetVillage, const std::string& casusBelli) {
    if (initiator == 0 || target == 0 || initiator == target) return 0;
    if (registry.getWarBetween(initiator, target) != nullptr) return 0;

    KingdomID initK = initiator;
    KingdomID tgtK = target;

    ActiveWar war;
    static uint32_t nextWarId = 1001;
    war.id = nextWarId++;
    war.attackerKingdom = initiator;
    war.defenderKingdom = target;
    war.targetVillageId = targetVillage;
    war.casusBelli = casusBelli.empty() ? "Conquest of County" : casusBelli;
    war.warScore = 0.f;
    war.battleScore = 0.f;
    war.occupationScore = 0.f;
    war.tickingScore = 0.f;
    war.tickingTimer = 0.f;
    war.startYear = registry.getYear();
    war.startDay = registry.getDay();
    war.isResolved = false;

    KingdomData* k1 = registry.getKingdom(initiator);
    KingdomData* k2 = registry.getKingdom(target);
    if (k1) k1->relations[target] = DiplomacyStatus::War;
    if (k2) k2->relations[initiator] = DiplomacyStatus::War;

    registry.registerWar(war);

    HistoricalRecord rec;
    rec.year = registry.getYear();
    rec.day = registry.getDay();
    std::string k1Name = k1 ? k1->name : "Realm " + std::to_string(initiator);
    std::string k2Name = k2 ? k2->name : "Realm " + std::to_string(target);
    rec.description = k1Name + " initiated war against " + k2Name + " (" + war.casusBelli + ").";
    registry.addHistory(rec);

    return war.id;
}

void WarfareManager::declareWar(SimulationRegistry& registry, KingdomID initiator, KingdomID target, const std::string& reason) {
    KingdomData* enemy = registry.getKingdom(target);
    VillageID targetV = (enemy && !enemy->controlledVillages.empty()) ? enemy->controlledVillages.front() : 0;
    declareWarWithGoal(registry, initiator, target, targetV, reason);
}

void WarfareManager::cancelWar(SimulationRegistry& registry, KingdomID k1, KingdomID k2) {
    ActiveWar* war = registry.getWarBetween(k1, k2);
    if (war) {
        signWhitePeace(registry, war->id);
    }
}

void WarfareManager::addBattleScore(SimulationRegistry& registry, KingdomID k1, KingdomID k2, float scoreDelta) {
    ActiveWar* war = registry.getWarBetween(k1, k2);
    if (!war) return;

    if (war->attackerKingdom == k1) war->battleScore += scoreDelta;
    else war->battleScore -= scoreDelta;

    war->battleScore = std::clamp(war->battleScore, -45.f, 45.f);
}

void WarfareManager::occupyVillage(SimulationRegistry& registry, uint32_t warId, VillageID vId, KingdomID occupier) {
    ActiveWar* war = registry.getWar(warId);
    if (!war) return;

    if (occupier == war->attackerKingdom) {
        war->occupiedByAttacker.insert(vId);
        war->occupiedByDefender.erase(vId);
    } else if (occupier == war->defenderKingdom) {
        war->occupiedByDefender.insert(vId);
        war->occupiedByAttacker.erase(vId);
    }

    registry.setCountyOccupier(vId, occupier);
}

void WarfareManager::liberateVillage(SimulationRegistry& registry, uint32_t warId, VillageID vId) {
    ActiveWar* war = registry.getWar(warId);
    if (!war) return;

    war->occupiedByAttacker.erase(vId);
    war->occupiedByDefender.erase(vId);
    registry.setCountyOccupier(vId, 0);
}

bool WarfareManager::enforceDemands(SimulationRegistry& registry, uint32_t warId) {
    ActiveWar* war = registry.getWar(warId);
    if (!war || war->isResolved) return false;

    KingdomData* attacker = registry.getKingdom(war->attackerKingdom);
    KingdomData* defender = registry.getKingdom(war->defenderKingdom);
    VillageData* targetV = registry.getVillage(war->targetVillageId);

    if (targetV && attacker && defender) {
        auto& dList = defender->controlledVillages;
        dList.erase(std::remove(dList.begin(), dList.end(), targetV->id), dList.end());

        targetV->kingdomId = attacker->id;
        attacker->controlledVillages.push_back(targetV->id);

        int foodRep = std::min(defender->treasuryFood, 60);
        int woodRep = std::min(defender->treasuryWood, 40);
        defender->treasuryFood -= foodRep;
        defender->treasuryWood -= woodRep;
        attacker->treasuryFood += foodRep;
        attacker->treasuryWood += woodRep;
    }

    for (VillageID vId : war->occupiedByAttacker) registry.setCountyOccupier(vId, 0);
    for (VillageID vId : war->occupiedByDefender) registry.setCountyOccupier(vId, 0);

    if (attacker) {
        attacker->relations[war->defenderKingdom] = DiplomacyStatus::Suspicious;
        attacker->truceYears[war->defenderKingdom] = registry.getYear() + 5;
    }
    if (defender) {
        defender->relations[war->attackerKingdom] = DiplomacyStatus::Suspicious;
        defender->truceYears[war->attackerKingdom] = registry.getYear() + 5;
    }

    war->isResolved = true;

    HistoricalRecord rec;
    rec.year = registry.getYear();
    rec.day = registry.getDay();
    rec.description = (attacker ? attacker->name : "Attacker") + " enforced demands in war against " +
                      (defender ? defender->name : "Defender") + ". Ceded " + (targetV ? targetV->name : "territory") + ".";
    registry.addHistory(rec);

    registry.removeResolvedWars();
    return true;
}

bool WarfareManager::signWhitePeace(SimulationRegistry& registry, uint32_t warId) {
    ActiveWar* war = registry.getWar(warId);
    if (!war || war->isResolved) return false;

    KingdomData* attacker = registry.getKingdom(war->attackerKingdom);
    KingdomData* defender = registry.getKingdom(war->defenderKingdom);

    for (VillageID vId : war->occupiedByAttacker) registry.setCountyOccupier(vId, 0);
    for (VillageID vId : war->occupiedByDefender) registry.setCountyOccupier(vId, 0);

    if (attacker) {
        attacker->relations[war->defenderKingdom] = DiplomacyStatus::Neutral;
        attacker->truceYears[war->defenderKingdom] = registry.getYear() + 3;
    }
    if (defender) {
        defender->relations[war->attackerKingdom] = DiplomacyStatus::Neutral;
        defender->truceYears[war->attackerKingdom] = registry.getYear() + 3;
    }

    war->isResolved = true;

    HistoricalRecord rec;
    rec.year = registry.getYear();
    rec.day = registry.getDay();
    rec.description = "White peace signed between " + (attacker ? attacker->name : "Attacker") + " and " + (defender ? defender->name : "Defender") + ".";
    registry.addHistory(rec);

    registry.removeResolvedWars();
    return true;
}

bool WarfareManager::surrenderWar(SimulationRegistry& registry, uint32_t warId, KingdomID surrenderingKingdom) {
    ActiveWar* war = registry.getWar(warId);
    if (!war || war->isResolved) return false;

    if (surrenderingKingdom == war->defenderKingdom) {
        return enforceDemands(registry, warId);
    }

    KingdomData* attacker = registry.getKingdom(war->attackerKingdom);
    KingdomData* defender = registry.getKingdom(war->defenderKingdom);

    if (attacker && defender) {
        int repFood = std::min(attacker->treasuryFood, 80);
        attacker->treasuryFood -= repFood;
        defender->treasuryFood += repFood;
    }

    for (VillageID vId : war->occupiedByAttacker) registry.setCountyOccupier(vId, 0);
    for (VillageID vId : war->occupiedByDefender) registry.setCountyOccupier(vId, 0);

    if (attacker) {
        attacker->relations[war->defenderKingdom] = DiplomacyStatus::Suspicious;
        attacker->truceYears[war->defenderKingdom] = registry.getYear() + 5;
    }
    if (defender) {
        defender->relations[war->attackerKingdom] = DiplomacyStatus::Suspicious;
        defender->truceYears[war->attackerKingdom] = registry.getYear() + 5;
    }

    war->isResolved = true;

    HistoricalRecord rec;
    rec.year = registry.getYear();
    rec.day = registry.getDay();
    rec.description = (attacker ? attacker->name : "Attacker") + " conceded unconditional surrender to " + (defender ? defender->name : "Defender") + ".";
    registry.addHistory(rec);

    registry.removeResolvedWars();
    return true;
}

void WarfareManager::updateWarProgression(SimulationRegistry& registry, float dt) {
    for (auto& war : registry.getAllActiveWars()) {
        if (war.isResolved) continue;

        KingdomData* defK = registry.getKingdom(war.defenderKingdom);
        KingdomData* attK = registry.getKingdom(war.attackerKingdom);

        float occScore = 0.f;
        if (war.targetVillageId != 0 && war.occupiedByAttacker.count(war.targetVillageId)) {
            occScore += 35.f;
        } else if (war.targetVillageId != 0 && war.occupiedByDefender.count(war.targetVillageId)) {
            occScore -= 35.f;
        }

        if (defK && defK->capitalVillageId != 0 && war.occupiedByAttacker.count(defK->capitalVillageId)) {
            occScore += 30.f;
        }
        if (attK && attK->capitalVillageId != 0 && war.occupiedByDefender.count(attK->capitalVillageId)) {
            occScore -= 30.f;
        }

        for (VillageID vId : war.occupiedByAttacker) {
            if (defK && vId != defK->capitalVillageId && vId != war.targetVillageId) occScore += 12.f;
        }
        for (VillageID vId : war.occupiedByDefender) {
            if (attK && vId != attK->capitalVillageId && vId != war.targetVillageId) occScore -= 12.f;
        }

        war.occupationScore = std::clamp(occScore, -60.f, 60.f);

        war.tickingTimer += dt;
        if (war.tickingTimer >= 10.f) {
            war.tickingTimer = 0.f;
            if (war.occupiedByAttacker.count(war.targetVillageId)) {
                war.tickingScore = std::min(war.tickingScore + 1.5f, 25.f);
            } else if (war.occupiedByDefender.empty() && war.occupiedByAttacker.empty()) {
                war.tickingScore = std::max(war.tickingScore - 1.0f, -25.f);
            }
        }

        war.warScore = std::clamp(war.battleScore + war.occupationScore + war.tickingScore, -100.f, 100.f);
    }
}

ArmyID WarfareManager::issueMusterOrder(SimulationRegistry& registry, KingdomID kingdomId, KingdomID targetKingdom, EntityID leaderId) {
    ArmyData army;
    army.id = registry.generateEntityId();
    army.homeKingdom = kingdomId;
    army.targetKingdom = targetKingdom;
    army.leaderId = leaderId;
    army.objective = ArmyObjective::Muster;

    KingdomData* kd = registry.getKingdom(kingdomId);
    if (kd) {
        army.worldX = kd->territoryMinX + (kd->territoryMaxX - kd->territoryMinX) * 0.5f;
        kd->activeArmies.push_back(army.id);
    }

    registry.registerArmy(army);
    return army.id;
}

ArmyID WarfareManager::declareRaid(SimulationRegistry& registry, KingdomID initiator, VillageID targetVillage, EntityID leaderId, const std::string& reason) {
    ArmyData raidArmy;
    raidArmy.id = registry.generateEntityId();
    raidArmy.homeKingdom = initiator;
    raidArmy.targetVillage = targetVillage;
    raidArmy.leaderId = leaderId;
    raidArmy.objective = ArmyObjective::Attack;

    VillageData* tv = registry.getVillage(targetVillage);
    if (tv) raidArmy.targetX = tv->centerX;

    registry.registerArmy(raidArmy);
    return raidArmy.id;
}

void WarfareManager::updateDiplomaticTension(SimulationRegistry&, uint64_t) {}
void WarfareManager::processArmies(SimulationRegistry&) {}

}