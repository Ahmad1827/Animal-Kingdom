#pragma once
#include <cstdint>

namespace sim {
    using EntityID = uint64_t;
    using DynastyID = uint64_t;
    using KingdomID = uint64_t;
    using VillageID = uint64_t;

    class IDGenerator {
    private:
        static inline EntityID nextEntityID = 1;
        static inline DynastyID nextDynastyID = 1;
        static inline KingdomID nextKingdomID = 1;
        static inline VillageID nextVillageID = 1;
    public:
        static EntityID generateEntityID() { return nextEntityID++; }
        static DynastyID generateDynastyID() { return nextDynastyID++; }
        static KingdomID generateKingdomID() { return nextKingdomID++; }
        static VillageID generateVillageID() { return nextVillageID++; }
    };
}