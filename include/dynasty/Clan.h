// include/dynasty/Clan.h
#pragma once
#include <string>
#include <unordered_map>
#include "dynasty/Character.h"
#include "dynasty/Dynasty.h"
#include "dynasty/Succession.h"

enum class CouncilPosition {
    NONE,
    WAR_CHANTER,
    FORAGER_CHIEF,
    WISE_ELDER,
    WHISPERER
};

struct ClanModifiers {
    float militaryMorale = 1.0f;
    float gatheringEfficiency = 1.0f;
    float stabilityModifier = 1.0f;
    float plotDetection = 1.0f;
};

class Clan {
public:
    uint64_t id = 0;
    std::string name;
    uint64_t dynastyId = 0;
    int tension = 10;
    SuccessionLaw successionLaw = SuccessionLaw::BLOODLINE_PRIMOGENITURE;

    std::unordered_map<CouncilPosition, Character::ID> council;

    void assignCouncil(CouncilPosition pos, Character::ID charId);
    Character::ID getCouncilMember(CouncilPosition pos) const;
    ClanModifiers calculateModifiers(const std::unordered_map<Character::ID, Character>& registry) const;
    void adjustTension(int delta);
};