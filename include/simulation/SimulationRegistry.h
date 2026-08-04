#pragma once
#include "simulation/ApeData.h"
#include "simulation/DynastyData.h"
#include "simulation/VillageData.h"
#include "simulation/ResourceNode.h"
#include <unordered_map>

namespace sim {
class SimulationRegistry {
private:
    std::unordered_map<EntityID, ApeData> apes;
    std::unordered_map<DynastyID, DynastyData> dynasties;
    std::unordered_map<VillageID, VillageData> villages;
    std::unordered_map<EntityID, ResourceNode> resources;

public:
    void registerApe(const ApeData& ape);
    void registerDynasty(const DynastyData& dyn);
    void registerVillage(const VillageData& village);
    void registerResource(const ResourceNode& res);

    ApeData* getApe(EntityID id);
    DynastyData* getDynasty(DynastyID id);
    VillageData* getVillage(VillageID id);
    ResourceNode* getResource(EntityID id);

    std::unordered_map<EntityID, ApeData>& getAllApes();
    std::unordered_map<DynastyID, DynastyData>& getAllDynasties();
    std::unordered_map<VillageID, VillageData>& getAllVillages();
    std::unordered_map<EntityID, ResourceNode>& getAllResources();
};
}