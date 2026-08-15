// src/dynasty/Character.cpp
#include "dynasty/Character.h"
#include <algorithm>

CharacterStats Character::getEffectiveStats() const {
    CharacterStats s = baseStats;
    for (TraitID trait : traits) {
        switch (trait) {
            case TraitID::SILVERBACK:
                s.prowess += 6;
                s.martial += 4;
                break;
            case TraitID::FIERCE_ROAR:
                s.prowess += 3;
                s.martial += 2;
                break;
            case TraitID::SNEAKY_FORAGER:
                s.intrigue += 5;
                s.stewardship += 2;
                break;
            case TraitID::WISE_ELDER:
                s.diplomacy += 6;
                s.stewardship += 2;
                break;
            case TraitID::NATURAL_LEADER:
                s.diplomacy += 4;
                s.martial += 2;
                break;
            case TraitID::COWARD:
                s.prowess = std::max(0, s.prowess - 5);
                s.martial = std::max(0, s.martial - 4);
                break;
            case TraitID::AMBITIOUS:
                s.intrigue += 2;
                s.martial += 2;
                break;
            case TraitID::LOYAL:
                s.diplomacy += 2;
                break;
            default:
                break;
        }
    }
    return s;
}

int Character::getOpinionOf(ID targetId) const {
    auto it = opinions.find(targetId);
    if (it != opinions.end()) {
        return it->second;
    }
    return 0;
}

void Character::setOpinionOf(ID targetId, int value) {
    opinions[targetId] = std::clamp(value, -100, 100);
}

void Character::modifyOpinionOf(ID targetId, int delta) {
    int current = getOpinionOf(targetId);
    setOpinionOf(targetId, current + delta);
}

bool Character::hasTrait(TraitID trait) const {
    return std::find(traits.begin(), traits.end(), trait) != traits.end();
}

void Character::addTrait(TraitID trait) {
    if (!hasTrait(trait)) {
        traits.push_back(trait);
    }
}