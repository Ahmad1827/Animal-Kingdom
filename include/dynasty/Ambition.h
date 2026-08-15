#pragma once
#include <string>
#include <cstdint>

namespace sim {

enum class AmbitionType {
    NONE,
    BECOME_ALPHA,
    BECOME_MARSHAL,
    BECOME_HEIR,
    EXPAND_PRESTIGE,
    SUPPORT_OFFSPRING,
    DISCREDIT_RIVAL
};

struct Ambition {
    AmbitionType type = AmbitionType::NONE;
    uint64_t targetId = 0;
    int progress = 0;

    std::string getName() const {
        switch (type) {
            case AmbitionType::BECOME_ALPHA: return "Claim Leadership of the Clan";
            case AmbitionType::BECOME_MARSHAL: return "Become Council War-Chanter";
            case AmbitionType::BECOME_HEIR: return "Secure Position as Designated Heir";
            case AmbitionType::EXPAND_PRESTIGE: return "Accumulate Tribal Prestige";
            case AmbitionType::SUPPORT_OFFSPRING: return "Advance Child's Standing";
            case AmbitionType::DISCREDIT_RIVAL: return "Undermine Clan Rival";
            default: return "Content with Standing";
        }
    }
};

}