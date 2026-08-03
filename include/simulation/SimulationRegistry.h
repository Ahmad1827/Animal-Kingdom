#pragma once
#include "simulation/ApeData.h"
#include "simulation/DynastyData.h"
#include "simulation/KingdomData.h"
#include <unordered_map>
#include <optional>

namespace sim {

class SimulationRegistry {
private:
    std::unordered_map<EntityID, ApeData> apes;
    std::unordered_map<DynastyID, DynastyData> dynasties;
    std::unordered_map<KingdomID, KingdomData> kingdoms;

public:
    void registerApe(const ApeData& ape);
    void registerDynasty(const DynastyData& dynasty);
    void registerKingdom(const KingdomData& kingdom);

    ApeData* getApe(EntityID id);
    DynastyData* getDynasty(DynastyID id);
    KingdomData* getKingdom(KingdomID id);

    std::unordered_map<EntityID, ApeData>& getAllApes();
    std::unordered_map<DynastyID, DynastyData>& getAllDynasties();
    std::unordered_map<KingdomID, KingdomData>& getAllKingdoms();
};

}