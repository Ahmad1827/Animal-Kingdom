#pragma once
#include <cstdint>

namespace sim {

using EntityID = uint64_t;
using DynastyID = uint64_t;
using KingdomID = uint64_t;

class IDGenerator {
private:
    static EntityID nextEntityID;
    static DynastyID nextDynastyID;
    static KingdomID nextKingdomID;

public:
    static EntityID generateEntityID();
    static DynastyID generateDynastyID();
    static KingdomID generateKingdomID();
};

}