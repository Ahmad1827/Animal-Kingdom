#include "simulation/SimulationRegistry.h"

namespace sim {

void SimulationRegistry::registerApe(const ApeData& ape) {
    apes[ape.id] = ape;
}

void SimulationRegistry::registerDynasty(const DynastyData& dynasty) {
    dynasties[dynasty.id] = dynasty;
}

void SimulationRegistry::registerKingdom(const KingdomData& kingdom) {
    kingdoms[kingdom.id] = kingdom;
}

ApeData* SimulationRegistry::getApe(EntityID id) {
    auto it = apes.find(id);
    if (it != apes.end()) {
        return &it->second;
    }
    return nullptr;
}

DynastyData* SimulationRegistry::getDynasty(DynastyID id) {
    auto it = dynasties.find(id);
    if (it != dynasties.end()) {
        return &it->second;
    }
    return nullptr;
}

KingdomData* SimulationRegistry::getKingdom(KingdomID id) {
    auto it = kingdoms.find(id);
    if (it != kingdoms.end()) {
        return &it->second;
    }
    return nullptr;
}

std::unordered_map<EntityID, ApeData>& SimulationRegistry::getAllApes() {
    return apes;
}

std::unordered_map<DynastyID, DynastyData>& SimulationRegistry::getAllDynasties() {
    return dynasties;
}

std::unordered_map<KingdomID, KingdomData>& SimulationRegistry::getAllKingdoms() {
    return kingdoms;
}

}