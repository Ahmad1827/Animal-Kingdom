// include/dynasty/Succession.h
#pragma once
#include <vector>
#include <string>
#include "dynasty/Character.h"
#include "dynasty/Dynasty.h"

enum class SuccessionLaw {
    BLOODLINE_PRIMOGENITURE,
    ELDER_SENIORITY,
    RIGHT_OF_THE_STRONGEST
};

struct SuccessionCandidate {
    Character::ID characterId;
    float score;
    std::string rationale;
};

class SuccessionSystem {
public:
    static std::vector<SuccessionCandidate> evaluateSuccession(
        const Dynasty& dynasty,
        const std::unordered_map<Character::ID, Character>& characterRegistry,
        SuccessionLaw law
    );

    static Character::ID determineHeir(
        const Dynasty& dynasty,
        const std::unordered_map<Character::ID, Character>& characterRegistry,
        SuccessionLaw law
    );
};