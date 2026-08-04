#include "simulation/SimulationRegistry.h"

namespace sim {
void SimulationRegistry::registerApe(const ApeData& ape) { apes[ape.id] = ape; }
void SimulationRegistry::registerDynasty(const DynastyData& dyn) { dynasties[dyn.id] = dyn; }
void SimulationRegistry::registerVillage(const VillageData& village) { villages[village.id] = village; }
void SimulationRegistry::registerResource(const ResourceNode& res) { resources[res.id] = res; }

ApeData* SimulationRegistry::getApe(EntityID id) { return apes.count(id) ? &apes[id] : nullptr; }
DynastyData* SimulationRegistry::getDynasty(DynastyID id) { return dynasties.count(id) ? &dynasties[id] : nullptr; }
VillageData* SimulationRegistry::getVillage(VillageID id) { return villages.count(id) ? &villages[id] : nullptr; }
ResourceNode* SimulationRegistry::getResource(EntityID id) { return resources.count(id) ? &resources[id] : nullptr; }

std::unordered_map<EntityID, ApeData>& SimulationRegistry::getAllApes() { return apes; }
std::unordered_map<DynastyID, DynastyData>& SimulationRegistry::getAllDynasties() { return dynasties; }
std::unordered_map<VillageID, VillageData>& SimulationRegistry::getAllVillages() { return villages; }
std::unordered_map<EntityID, ResourceNode>& SimulationRegistry::getAllResources() { return resources; }
}