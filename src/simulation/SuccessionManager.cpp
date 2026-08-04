#include "simulation/SuccessionManager.h"

namespace sim {

EntityID SuccessionManager::findNextHeir(SimulationRegistry& registry, EntityID currentLeaderId) {
    ApeData* leader = registry.getApe(currentLeaderId);
    if (!leader) return 0;

    for (EntityID childId : leader->children) {
        ApeData* child = registry.getApe(childId);
        if (child && child->alive) return childId;
    }

    for (EntityID sibId : leader->siblings) {
        ApeData* sib = registry.getApe(sibId);
        if (sib && sib->alive) return sibId;
    }

    DynastyData* dynasty = registry.getDynasty(leader->dynastyId);
    if (dynasty) {
        for (EntityID memId : dynasty->members) {
            if (memId == currentLeaderId) continue;
            ApeData* mem = registry.getApe(memId);
            if (mem && mem->alive) return memId;
        }
    }

    return 0;
}

}