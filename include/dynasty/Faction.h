#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "dynasty/Character.h"

namespace sim {

enum class FactionType {
    CLAIMANT_FOR_ALPHA,
    CHANGE_LAW_SENIORITY,
    CHANGE_LAW_STRONGEST,
    COUNCIL_MALCONTENT
};

class Faction {
public:
    uint64_t id = 0;
    FactionType type;
    std::string name;
    Character::ID leaderId = Character::INVALID_ID;
    Character::ID targetClaimantId = Character::INVALID_ID;
    std::vector<Character::ID> memberIds;
    float powerRating = 0.0f;

    void addMember(Character::ID memberId);
    void removeMember(Character::ID memberId);
    bool hasMember(Character::ID memberId) const;
    void recalculatePower(const std::unordered_map<Character::ID, Character>& registry);
};

}