#pragma once
#include "simulation/SimulationRegistry.h"
#include <string>

namespace sim {

class WarfareManager {
public:
    static void update(SimulationRegistry& registry, float dt);
    static uint32_t declareWarWithGoal(SimulationRegistry& registry, KingdomID initiator, KingdomID target, VillageID targetVillage, const std::string& casusBelli);
    static void declareWar(SimulationRegistry& registry, KingdomID initiator, KingdomID target, const std::string& reason);
    static void cancelWar(SimulationRegistry& registry, KingdomID k1, KingdomID k2);
    static void addBattleScore(SimulationRegistry& registry, KingdomID k1, KingdomID k2, float scoreDelta);
    static void occupyVillage(SimulationRegistry& registry, uint32_t warId, VillageID vId, KingdomID occupier);
    static void liberateVillage(SimulationRegistry& registry, uint32_t warId, VillageID vId);
    static bool enforceDemands(SimulationRegistry& registry, uint32_t warId);
    static bool signWhitePeace(SimulationRegistry& registry, uint32_t warId);
    static bool surrenderWar(SimulationRegistry& registry, uint32_t warId, KingdomID surrenderingKingdom);
    static ArmyID issueMusterOrder(SimulationRegistry& registry, KingdomID kingdomId, KingdomID targetKingdom, EntityID leaderId);
    static ArmyID declareRaid(SimulationRegistry& registry, KingdomID initiator, VillageID targetVillage, EntityID leaderId, const std::string& reason);

private:
    static void updateWarProgression(SimulationRegistry& registry, float dt);
    static void updateDiplomaticTension(SimulationRegistry& registry, uint64_t ticks);
    static void processArmies(SimulationRegistry& registry);
};

}