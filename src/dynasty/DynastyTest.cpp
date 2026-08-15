#include "dynasty/DynastyTest.h"
#include "dynasty/Succession.h"
#include <cassert>
#include <iostream>

namespace sim {

void DynastyTestRunner::runDeterministicTest() {
    std::unordered_map<Character::ID, Character> registry;
    Dynasty dynasty;
    dynasty.id = 1;
    dynasty.name = "First Tree";

    Clan clan;
    clan.id = 1;
    clan.name = "Grove Clan";
    clan.dynastyId = dynasty.id;

    Character koba;
    koba.id = 101;
    koba.name = "Koba";
    koba.age = 38;
    koba.sex = Sex::MALE;
    koba.dynastyId = dynasty.id;
    koba.addTrait(TraitID::SILVERBACK);
    koba.baseStats.prowess = 14;
    koba.baseStats.martial = 12;

    Character maya;
    maya.id = 102;
    maya.name = "Maya";
    maya.age = 34;
    maya.sex = Sex::FEMALE;
    maya.dynastyId = dynasty.id;
    maya.addTrait(TraitID::WISE_ELDER);

    Character tano;
    tano.id = 103;
    tano.name = "Tano";
    tano.age = 16;
    tano.sex = Sex::MALE;
    tano.fatherId = koba.id;
    tano.motherId = maya.id;
    tano.dynastyId = dynasty.id;
    tano.addTrait(TraitID::NATURAL_LEADER);

    Character raku;
    raku.id = 104;
    raku.name = "Raku";
    raku.age = 14;
    raku.sex = Sex::MALE;
    raku.fatherId = koba.id;
    raku.motherId = maya.id;
    raku.dynastyId = dynasty.id;
    raku.addTrait(TraitID::SNEAKY_FORAGER);

    Character sela;
    sela.id = 105;
    sela.name = "Sela";
    sela.age = 11;
    sela.sex = Sex::FEMALE;
    sela.fatherId = koba.id;
    sela.motherId = maya.id;
    sela.dynastyId = dynasty.id;

    Character boro;
    boro.id = 106;
    boro.name = "Boro";
    boro.age = 44;
    boro.sex = Sex::MALE;
    boro.dynastyId = dynasty.id;
    boro.addTrait(TraitID::AMBITIOUS);
    boro.addTrait(TraitID::FIERCE_ROAR);
    boro.baseStats.prowess = 18;
    boro.baseStats.martial = 15;

    koba.spouseIds.push_back(maya.id);
    maya.spouseIds.push_back(koba.id);
    koba.childrenIds = { tano.id, raku.id, sela.id };
    maya.childrenIds = { tano.id, raku.id, sela.id };

    registry[koba.id] = koba;
    registry[maya.id] = maya;
    registry[tano.id] = tano;
    registry[raku.id] = raku;
    registry[sela.id] = sela;
    registry[boro.id] = boro;

    dynasty.registerMember(koba.id);
    dynasty.registerMember(maya.id);
    dynasty.registerMember(tano.id);
    dynasty.registerMember(raku.id);
    dynasty.registerMember(sela.id);
    dynasty.registerMember(boro.id);
    dynasty.founderId = koba.id;
    dynasty.setAlpha(koba.id);

    clan.successionLaw = SuccessionLaw::BLOODLINE_PRIMOGENITURE;
    Character::ID heir1 = SuccessionSystem::determineHeir(dynasty, registry, clan.successionLaw);
    assert(heir1 == tano.id);

    registry[koba.id].isAlive = false;
    dynasty.setAlpha(heir1);
    assert(dynasty.currentAlphaId == tano.id);
    assert(dynasty.historicalAlphaIds.size() == 1);
    assert(dynasty.historicalAlphaIds.front() == koba.id);

    clan.successionLaw = SuccessionLaw::ELDER_SENIORITY;
    Character::ID heirSeniority = SuccessionSystem::determineHeir(dynasty, registry, clan.successionLaw);
    assert(heirSeniority == boro.id);

    clan.successionLaw = SuccessionLaw::RIGHT_OF_THE_STRONGEST;
    Character::ID heirStrongest = SuccessionSystem::determineHeir(dynasty, registry, clan.successionLaw);
    assert(heirStrongest == boro.id);
}

}