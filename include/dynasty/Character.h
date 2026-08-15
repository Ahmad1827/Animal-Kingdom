// include/dynasty/Character.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

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
    int ambition = 50;
    std::unordered_map<ID, int> opinions;

    CharacterStats getEffectiveStats() const;
    int getOpinionOf(ID targetId) const;
    void setOpinionOf(ID targetId, int value);
    void modifyOpinionOf(ID targetId, int delta);
    bool hasTrait(TraitID trait) const;
    void addTrait(TraitID trait);
};