#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "dynasty/Opinion.h"
#include "dynasty/Ambition.h"
#include "dynasty/Claim.h"

namespace sim {

enum class Sex { MALE, FEMALE };

enum class TraitID {
    SILVERBACK,
    FIERCE_ROAR,
    SNEAKY_FORAGER,
    WISE_ELDER,
    FICKLE_GROOMER,
    NATURAL_LEADER,
    COWARD,
    AMBITIOUS,
    LOYAL
};

struct CharacterStats {
    int prowess = 10;
    int martial = 10;
    int stewardship = 10;
    int intrigue = 10;
    int diplomacy = 10;
    int health = 100;
    int fertility = 50;
};

struct CharacterHistoryEntry {
    int year;
    int day;
    std::string description;
};

class Character {
public:
    using ID = uint64_t;
    static constexpr ID INVALID_ID = 0;

    ID id = INVALID_ID;
    std::string name;
    Sex sex = Sex::MALE;
    int age = 0;
    bool isAlive = true;

    uint64_t clanId = 0;
    uint64_t dynastyId = 0;

    ID fatherId = INVALID_ID;
    ID motherId = INVALID_ID;
    std::vector<ID> spouseIds;
    std::vector<ID> childrenIds;

    CharacterStats baseStats;
    std::vector<TraitID> traits;
    
    int loyalty = 50;
    int ambitionScore = 50;
    int prestige = 0;

    Ambition ambition;
    std::vector<Claim> claims;
    std::unordered_map<ID, OpinionMatrix> opinions;
    std::vector<CharacterHistoryEntry> history;

    CharacterStats getEffectiveStats() const;
    int getOpinionOf(ID targetId) const;
    const OpinionMatrix* getOpinionBreakdown(ID targetId) const;
    void addOpinionModifier(ID targetId, const std::string& reason, int value, float duration = -1.0f);
    void removeOpinionModifier(ID targetId, const std::string& reason);
    bool hasTrait(TraitID trait) const;
    void addTrait(TraitID trait);
    void logHistory(int year, int day, const std::string& desc);
};

}