// src/dynasty/Clan.cpp
#include "dynasty/Clan.h"
#include <algorithm>

void Clan::assignCouncil(CouncilPosition pos, Character::ID charId) {
    council[pos] = charId;
}

Character::ID Clan::getCouncilMember(CouncilPosition pos) const {
    auto it = council.find(pos);
    if (it != council.end()) return it->second;
    return Character::INVALID_ID;
}

ClanModifiers Clan::calculateModifiers(const std::unordered_map<Character::ID, Character>& registry) const {
    ClanModifiers mods;

    auto warId = getCouncilMember(CouncilPosition::WAR_CHANTER);
    if (warId != Character::INVALID_ID && registry.count(warId)) {
        CharacterStats s = registry.at(warId).getEffectiveStats();
        mods.militaryMorale += (s.martial + s.prowess) * 0.02f;
    }

    auto foragerId = getCouncilMember(CouncilPosition::FORAGER_CHIEF);
    if (foragerId != Character::INVALID_ID && registry.count(foragerId)) {
        CharacterStats s = registry.at(foragerId).getEffectiveStats();
        mods.gatheringEfficiency += s.stewardship * 0.03f;
    }

    auto elderId = getCouncilMember(CouncilPosition::WISE_ELDER);
    if (elderId != Character::INVALID_ID && registry.count(elderId)) {
        CharacterStats s = registry.at(elderId).getEffectiveStats();
        mods.stabilityModifier += s.diplomacy * 0.025f;
    }

    auto whisperId = getCouncilMember(CouncilPosition::WHISPERER);
    if (whisperId != Character::INVALID_ID && registry.count(whisperId)) {
        CharacterStats s = registry.at(whisperId).getEffectiveStats();
        mods.plotDetection += s.intrigue * 0.04f;
    }

    return mods;
}

void Clan::adjustTension(int delta) {
    tension = std::clamp(tension + delta, 0, 100);
}