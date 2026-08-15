#include "dynasty/Succession.h"
#include <algorithm>

namespace sim {

std::vector<SuccessionCandidate> SuccessionSystem::evaluateSuccession(
    const Dynasty& dynasty,
    const std::unordered_map<Character::ID, Character>& characterRegistry,
    const std::vector<Faction>& factions,
    SuccessionLaw law
) {
    std::vector<SuccessionCandidate> candidates;

    auto alphaIt = characterRegistry.find(dynasty.currentAlphaId);
    const Character* currentAlpha = (alphaIt != characterRegistry.end()) ? &alphaIt->second : nullptr;

    for (Character::ID memberId : dynasty.memberIds) {
        auto it = characterRegistry.find(memberId);
        if (it == characterRegistry.end()) continue;
        const Character& candidate = it->second;

        if (!candidate.isAlive || candidate.id == dynasty.currentAlphaId) {
            continue;
        }

        SuccessionCandidate entry;
        entry.characterId = candidate.id;
        entry.score = 0.0f;

        for (const auto& f : factions) {
            if (f.targetClaimantId == candidate.id) {
                entry.factionBackingPower += f.powerRating;
            }
        }

        switch (law) {
            case SuccessionLaw::BLOODLINE_PRIMOGENITURE: {
                if (currentAlpha && std::find(currentAlpha->childrenIds.begin(), currentAlpha->childrenIds.end(), candidate.id) != currentAlpha->childrenIds.end()) {
                    entry.score += 1000.0f + (100.0f - candidate.age * 0.1f);
                    entry.rationale = "Direct Offspring (Primogeniture)";
                } else if (candidate.fatherId == (currentAlpha ? currentAlpha->fatherId : 0) && candidate.fatherId != 0) {
                    entry.score += 500.0f + candidate.age;
                    entry.rationale = "Sibling of Alpha";
                } else {
                    entry.score += 10.0f + candidate.age;
                    entry.rationale = "Dynasty Kin";
                }
                break;
            }
            case SuccessionLaw::ELDER_SENIORITY: {
                entry.score = static_cast<float>(candidate.age * 10);
                entry.rationale = "Seniority Age: " + std::to_string(candidate.age);
                break;
            }
            case SuccessionLaw::RIGHT_OF_THE_STRONGEST: {
                CharacterStats stats = candidate.getEffectiveStats();
                entry.score = static_cast<float>(stats.prowess * 5 + stats.martial * 3);
                if (candidate.hasTrait(TraitID::SILVERBACK)) entry.score += 50.0f;
                if (candidate.hasTrait(TraitID::FIERCE_ROAR)) entry.score += 30.0f;
                if (candidate.hasTrait(TraitID::COWARD)) entry.score -= 60.0f;
                entry.rationale = "Prowess " + std::to_string(stats.prowess) + ", Martial " + std::to_string(stats.martial);
                break;
            }
        }

        entry.score += entry.factionBackingPower * 0.5f;
        candidates.push_back(entry);
    }

    std::sort(candidates.begin(), candidates.end(), [](const SuccessionCandidate& a, const SuccessionCandidate& b) {
        return a.score > b.score;
    });

    return candidates;
}

Character::ID SuccessionSystem::determineHeir(
    const Dynasty& dynasty,
    const std::unordered_map<Character::ID, Character>& characterRegistry,
    const std::vector<Faction>& factions,
    SuccessionLaw law
) {
    auto candidates = evaluateSuccession(dynasty, characterRegistry, factions, law);
    if (!candidates.empty()) {
        return candidates.front().characterId;
    }
    return Character::INVALID_ID;
}

}