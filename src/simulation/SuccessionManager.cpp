#include "simulation/SuccessionManager.h"
#include "simulation/SimulationRegistry.h"
#include <algorithm>

namespace sim {

EntityID SuccessionManager::findNextHeir(SimulationRegistry& registry, EntityID currentLeaderId) {
    ApeData* leader = registry.getApe(currentLeaderId);
    if (!leader) return 0;

    for (EntityID childId : leader->children) {
        ApeData* child = registry.getApe(childId);
        if (child && child->alive) return childId;
    }

    for (EntityID siblingId : leader->siblings) {
        ApeData* sibling = registry.getApe(siblingId);
        if (sibling && sibling->alive) return siblingId;
    }

    DynastyData* dyn = registry.getDynasty(leader->dynastyId);
    if (dyn) {
        for (EntityID memberId : dyn->members) {
            ApeData* member = registry.getApe(memberId);
            if (member && member->alive && memberId != currentLeaderId) return memberId;
        }
    }

    return 0;
}

void SuccessionManager::handleDeath(SimulationRegistry& registry, EntityID deceasedId) {
    ApeData* deceased = registry.getApe(deceasedId);
    if (!deceased) return;

    deceased->alive = false;

    HistoricalRecord deathRec;
    deathRec.year = registry.getYear();
    deathRec.day = registry.getDay();
    deathRec.description = deceased->name + " died.";
    registry.addHistory(deathRec);

    DynastyData* dyn = registry.getDynasty(deceased->dynastyId);
    if (dyn && dyn->currentLeaderId == deceasedId) {
        EntityID heirId = findNextHeir(registry, deceasedId);
        if (heirId != 0) {
            dyn->currentLeaderId = heirId;
            ApeData* heir = registry.getApe(heirId);
            if (heir) {
                HistoricalRecord succRec;
                succRec.year = registry.getYear();
                succRec.day = registry.getDay();
                succRec.description = heir->name + " inherited leadership of Dynasty " + dyn->name + ".";
                registry.addHistory(succRec);
            }
        }
    }

    if (deceased->currentKingdom != 0) {
        KingdomData* kd = registry.getKingdom(deceased->currentKingdom);
        if (kd && kd->currentKingId == deceasedId) {
            EntityID heirId = findNextHeir(registry, deceasedId);
            if (heirId != 0) {
                kd->currentKingId = heirId;
                ApeData* heir = registry.getApe(heirId);
                if (heir) {
                    heir->currentKingdom = kd->id;
                    HistoricalRecord kingRec;
                    kingRec.year = registry.getYear();
                    kingRec.day = registry.getDay();
                    kingRec.description = heir->name + " was crowned King of " + kd->name + ".";
                    registry.addHistory(kingRec);
                }
            } else {
                HistoricalRecord colRec;
                colRec.year = registry.getYear();
                colRec.day = registry.getDay();
                colRec.description = kd->name + " collapsed due to lack of an heir.";
                registry.addHistory(colRec);
            }
        }
    }
}

}