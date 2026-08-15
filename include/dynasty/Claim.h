#pragma once
#include <string>
#include <cstdint>

namespace sim {

enum class ClaimStrength {
    STRONG,
    WEAK
};

struct Claim {
    uint64_t dynastyId;
    ClaimStrength strength;
    std::string reason;
};

}