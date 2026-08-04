#pragma once
#include "simulation/SimulationRegistry.h"

namespace sim {

class KingdomManager {
public:
    static void update(SimulationRegistry& registry, uint64_t ticks);
    static void handleFirstContact(SimulationRegistry& registry, KingdomID k1, KingdomID k2);
    static void spawnDebugKingdom(SimulationRegistry& registry, VillageID capitalId, DynastyID dynastyId, EntityID kingId);
private:
    static void collectTribute(SimulationRegistry& registry, KingdomData& kingdom);
    static void updateInfluence(SimulationRegistry& registry, KingdomData& kingdom);
    static void updateDiplomacy(SimulationRegistry& registry, KingdomData& kingdom, uint64_t ticks);
};

}