#include "dynasty/Character.h"
#include <algorithm>

namespace sim {

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
                s.intrigue += 3;
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
        return it->second.calculateTotal();
    }
    return 0;
}

const OpinionMatrix* Character::getOpinionBreakdown(ID targetId) const {
    auto it = opinions.find(targetId);
    if (it != opinions.end()) {
        return &it->second;
    }
    return nullptr;
}

void Character::addOpinionModifier(ID targetId, const std::string& reason, int value, float duration) {
    opinions[targetId].addModifier(reason, value, duration);
}

void Character::removeOpinionModifier(ID targetId, const std::string& reason) {
    auto it = opinions.find(targetId);
    if (it != opinions.end()) {
        it->second.removeModifier(reason);
    }
}

bool Character::hasTrait(TraitID trait) const {
    return std::find(traits.begin(), traits.end(), trait) != traits.end();
}

void Character::addTrait(TraitID trait) {
    if (!hasTrait(trait)) {
        traits.push_back(trait);
    }
}

void Character::logHistory(int year, int day, const std::string& desc) {
    history.push_back({year, day, desc});
}

}