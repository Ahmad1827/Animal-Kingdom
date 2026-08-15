#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "dynasty/Character.h"
#include "dynasty/Dynasty.h"
#include "dynasty/Faction.h"

namespace sim {

enum class SuccessionLaw {
    BLOODLINE_PRIMOGENITURE,
    ELDER_SENIORITY,
    RIGHT_OF_THE_STRONGEST
};

struct SuccessionCandidate {
    Character::ID characterId;
    float score;
    std::string rationale;
    float factionBackingPower = 0.0f;
};

class SuccessionSystem {
public:
    static std::vector<SuccessionCandidate> evaluateSuccession(
        const Dynasty& dynasty,
        const std::unordered_map<Character::ID, Character>& characterRegistry,
        const std::vector<Faction>& factions,
        SuccessionLaw law
    );

    static Character::ID determineHeir(
        const Dynasty& dynasty,
        const std::unordered_map<Character::ID, Character>& characterRegistry,
        const std::vector<Faction>& factions,
        SuccessionLaw law
    );
};

}