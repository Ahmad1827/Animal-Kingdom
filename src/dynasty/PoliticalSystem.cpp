#include "dynasty/PoliticalSystem.h"
#include <cmath>
#include <algorithm>

namespace sim {

void PoliticalSystem::updatePoliticalAI(
    Clan& clan,
    const Dynasty& dynasty,
    std::unordered_map<Character::ID, Character>& registry,
    std::vector<Faction>& factions,
    int year,
    int day
) {
    Character::ID alphaId = dynasty.currentAlphaId;
    if (alphaId == Character::INVALID_ID || !registry.count(alphaId)) return;

    const Character& alpha = registry.at(alphaId);

    for (auto& pair : registry) {
        Character& ape = pair.second;
        if (!ape.isAlive || ape.id == alphaId) continue;

        int opinionOfAlpha = ape.getOpinionOf(alphaId);

        if (ape.hasTrait(TraitID::AMBITIOUS) && ape.sex == Sex::MALE) {
            ape.ambition.type = AmbitionType::BECOME_ALPHA;
        } else if (ape.hasTrait(TraitID::SILVERBACK)) {
            ape.ambition.type = AmbitionType::BECOME_MARSHAL;
        }

        if (opinionOfAlpha < -20 || ape.hasTrait(TraitID::AMBITIOUS)) {
            bool inClaimantFaction = false;
            for (auto& f : factions) {
                if (f.type == FactionType::CLAIMANT_FOR_ALPHA) {
                    if (f.targetClaimantId == ape.id || f.leaderId == ape.id) {
                        inClaimantFaction = true;
                        break;
                    }
                }
            }

            if (!inClaimantFaction && ape.hasTrait(TraitID::AMBITIOUS) && ape.getEffectiveStats().prowess > alpha.getEffectiveStats().prowess) {
                Faction newFact;
                newFact.id = static_cast<uint64_t>(factions.size() + 101);
                newFact.type = FactionType::CLAIMANT_FOR_ALPHA;
                newFact.name = ape.name + "'s Claim on Leadership";
                newFact.leaderId = ape.id;
                newFact.targetClaimantId = ape.id;
                newFact.addMember(ape.id);
                factions.push_back(newFact);
                ape.logHistory(year, day, "Formed faction to claim Alpha leadership.");
            }
        }

        for (auto& f : factions) {
            if (f.type == FactionType::CLAIMANT_FOR_ALPHA && f.targetClaimantId != ape.id) {
                int opClaimant = ape.getOpinionOf(f.targetClaimantId);
                if (opClaimant > opinionOfAlpha + 25 && !ape.hasTrait(TraitID::LOYAL)) {
                    if (!f.hasMember(ape.id)) {
                        f.addMember(ape.id);
                        ape.logHistory(year, day, "Joined " + f.name);
                    }
                } else if (opinionOfAlpha > 0 || ape.hasTrait(TraitID::LOYAL)) {
                    if (f.hasMember(ape.id)) {
                        f.removeMember(ape.id);
                        ape.logHistory(year, day, "Abandoned faction: " + f.name);
                    }
                }
            }
        }
    }

    for (auto& f : factions) {
        f.recalculatePower(registry);
    }

    factions.erase(
        std::remove_if(factions.begin(), factions.end(), [&](const Faction& f) {
            return f.memberIds.empty() || (registry.count(f.leaderId) && !registry.at(f.leaderId).isAlive);
        }),
        factions.end()
    );

    clan.tension = calculateEmergentTension(clan, dynasty, registry, factions);
}

int PoliticalSystem::calculateEmergentTension(
    const Clan& clan,
    const Dynasty& dynasty,
    const std::unordered_map<Character::ID, Character>& registry,
    const std::vector<Faction>& factions
) {
    float tension = 5.0f;
    Character::ID alphaId = dynasty.currentAlphaId;
    if (alphaId == Character::INVALID_ID || !registry.count(alphaId)) return 50;

    const Character& alpha = registry.at(alphaId);
    CharacterStats alphaStats = alpha.getEffectiveStats();

    int totalOpinion = 0;
    int livingCount = 0;

    for (Character::ID mid : dynasty.memberIds) {
        auto it = registry.find(mid);
        if (it != registry.end() && it->second.isAlive && mid != alphaId) {
            totalOpinion += it->second.getOpinionOf(alphaId);
            livingCount++;
        }
    }

    if (livingCount > 0) {
        float avgOpinion = static_cast<float>(totalOpinion) / static_cast<float>(livingCount);
        if (avgOpinion < 0.0f) {
            tension += std::abs(avgOpinion) * 0.5f;
        }
    }

    float totalFactionPower = 0.0f;
    for (const auto& f : factions) {
        totalFactionPower += f.powerRating;
    }

    float alphaPower = static_cast<float>(alphaStats.prowess * 4 + alphaStats.martial * 3 + alpha.prestige / 5);
    if (totalFactionPower > alphaPower) {
        tension += (totalFactionPower - alphaPower) * 0.4f;
    }

    auto marshalId = clan.getCouncilMember(CouncilPosition::WAR_CHANTER);
    if (marshalId != Character::INVALID_ID && registry.count(marshalId)) {
        if (registry.at(marshalId).getOpinionOf(alphaId) < -15) {
            tension += 15.0f;
        }
    }

    return std::clamp(static_cast<int>(tension), 0, 100);
}

}