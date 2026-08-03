#include "simulation/EntityID.h"

namespace sim {

EntityID IDGenerator::nextEntityID = 1;
DynastyID IDGenerator::nextDynastyID = 1;
KingdomID IDGenerator::nextKingdomID = 1;

EntityID IDGenerator::generateEntityID() {
    return nextEntityID++;
}

DynastyID IDGenerator::generateDynastyID() {
    return nextDynastyID++;
}

KingdomID IDGenerator::generateKingdomID() {
    return nextKingdomID++;
}

}