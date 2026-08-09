#pragma once
#include "simulation/SimulationRegistry.h"
#include <string>

namespace sim {

class WarfareManager {
public:
    static void update(SimulationRegistry& registry, uint64_t ticks);
    static void declareWar(SimulationRegistry& registry, KingdomID initiator, KingdomID target, const std::string& reason);
    static void cancelWar(SimulationRegistry& registry, KingdomID k1, KingdomID k2);
    static ArmyID issueMusterOrder(SimulationRegistry& registry, KingdomID kingdomId, KingdomID targetKingdom, EntityID leaderId);
    static ArmyID declareRaid(SimulationRegistry& registry, KingdomID initiator, VillageID targetVillage, EntityID leaderId, const std::string& reason);
private:
    static void updateDiplomaticTension(SimulationRegistry& registry, uint64_t ticks);
    static void processArmies(SimulationRegistry& registry);
};

}