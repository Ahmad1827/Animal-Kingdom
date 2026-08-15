#pragma once
#include <vector>
#include <unordered_map>
#include "dynasty/Character.h"
#include "dynasty/Dynasty.h"
#include "dynasty/Clan.h"
#include "dynasty/Faction.h"

namespace sim {

class PoliticalSystem {
public:
    static void updatePoliticalAI(
        Clan& clan,
        const Dynasty& dynasty,
        std::unordered_map<Character::ID, Character>& registry,
        std::vector<Faction>& factions,
        int year,
        int day
    );

    static int calculateEmergentTension(
        const Clan& clan,
        const Dynasty& dynasty,
        const std::unordered_map<Character::ID, Character>& registry,
        const std::vector<Faction>& factions
    );
};

}