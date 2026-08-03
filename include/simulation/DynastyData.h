#pragma once
#include "simulation/EntityID.h"
#include <string>
#include <vector>

namespace sim {

struct DynastyData {
    DynastyID id;
    std::string name;
    EntityID founderId;
    EntityID currentLeaderId;
    EntityID primaryHeirId;
    int wealth;
    int prestige;
    int legitimacy;
    std::vector<EntityID> members;
    std::vector<KingdomID> kingdomIds;
};

}