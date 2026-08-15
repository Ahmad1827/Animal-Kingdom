#include "dynasty/Faction.h"
#include <algorithm>

namespace sim {

void Faction::addMember(Character::ID memberId) {
    if (!hasMember(memberId)) {
        memberIds.push_back(memberId);
    }
}

void Faction::removeMember(Character::ID memberId) {
    memberIds.erase(std::remove(memberIds.begin(), memberIds.end(), memberId), memberIds.end());
}

bool Faction::hasMember(Character::ID memberId) const {
    return std::find(memberIds.begin(), memberIds.end(), memberId) != memberIds.end();
}

void Faction::recalculatePower(const std::unordered_map<Character::ID, Character>& registry) {
    powerRating = 0.0f;
    for (Character::ID mid : memberIds) {
        auto it = registry.find(mid);
        if (it != registry.end() && it->second.isAlive) {
            CharacterStats s = it->second.getEffectiveStats();
            powerRating += static_cast<float>(s.prowess * 3 + s.martial * 2 + it->second.prestige / 10);
        }
    }
}

}