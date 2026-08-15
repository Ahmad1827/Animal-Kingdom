#include "dynasty/DynastyUI.h"
#include "dynasty/Succession.h"

namespace sim {

DynastyUI::DynastyUI() {
    panelBg.setFillColor(sf::Color(18, 14, 10, 240));
    panelBg.setOutlineColor(sf::Color(170, 130, 70));
    panelBg.setOutlineThickness(2.f);
}

void DynastyUI::init(const sf::Font& loadedFont) {
    font = loadedFont;
    headerText.setFont(font);
    headerText.setCharacterSize(22);
    headerText.setFillColor(sf::Color(245, 210, 110));

    detailText.setFont(font);
    detailText.setCharacterSize(15);
    detailText.setFillColor(sf::Color(225, 225, 225));
}

void DynastyUI::toggle(DynastyUIMode mode) {
    if (currentMode == mode) {
        currentMode = DynastyUIMode::CLOSED;
    } else {
        currentMode = mode;
    }
}

void DynastyUI::close() {
    currentMode = DynastyUIMode::CLOSED;
}

void DynastyUI::nextCharacter(const Dynasty& dynasty) {
    if (!dynasty.memberIds.empty()) {
        inspectedMemberIndex = (inspectedMemberIndex + 1) % dynasty.memberIds.size();
    }
}

void DynastyUI::previousCharacter(const Dynasty& dynasty) {
    if (!dynasty.memberIds.empty()) {
        inspectedMemberIndex = (inspectedMemberIndex + dynasty.memberIds.size() - 1) % dynasty.memberIds.size();
    }
}

void DynastyUI::render(
    sf::RenderTarget& target,
    const Dynasty& dynasty,
    const Clan& clan,
    const std::unordered_map<Character::ID, Character>& registry,
    const std::vector<Faction>& factions,
    Character::ID currentAlphaId
) {
    if (currentMode == DynastyUIMode::CLOSED) return;

    sf::Vector2f viewSize = target.getView().getSize();
    panelBg.setSize(sf::Vector2f(viewSize.x * 0.78f, viewSize.y * 0.78f));
    panelBg.setOrigin(panelBg.getSize().x / 2.f, panelBg.getSize().y / 2.f);
    panelBg.setPosition(target.getView().getCenter());

    target.draw(panelBg);

    switch (currentMode) {
        case DynastyUIMode::CHARACTER_VIEW: {
            if (!dynasty.memberIds.empty()) {
                if (inspectedMemberIndex >= dynasty.memberIds.size()) inspectedMemberIndex = 0;
                Character::ID targetCharId = dynasty.memberIds[inspectedMemberIndex];
                if (registry.count(targetCharId)) {
                    drawCharacterView(target, registry.at(targetCharId), dynasty, clan, currentAlphaId);
                }
            }
            break;
        }
        case DynastyUIMode::FAMILY_TREE_VIEW:
            drawFamilyTreeView(target, dynasty, registry, currentAlphaId);
            break;
        case DynastyUIMode::SUCCESSION_VIEW:
            drawSuccessionView(target, dynasty, clan, registry, factions);
            break;
        default:
            break;
    }
}

void DynastyUI::drawCharacterView(
    sf::RenderTarget& target,
    const Character& character,
    const Dynasty& dynasty,
    const Clan& clan,
    Character::ID alphaId
) {
    sf::Vector2f center = panelBg.getPosition();
    float startX = center.x - panelBg.getSize().x * 0.46f;
    float startY = center.y - panelBg.getSize().y * 0.45f;

    std::string roleLabel = (character.id == alphaId) ? " [ALPHA RULER]" : " [CLAN MEMBER]";
    headerText.setString("APE DOSSIER: " + character.name + " (" + dynasty.name + ")" + roleLabel);
    headerText.setPosition(startX, startY);
    target.draw(headerText);

    CharacterStats stats = character.getEffectiveStats();

    std::string info =
        "Status: " + std::string(character.isAlive ? "ALIVE" : "DEAD") + " | Age: " + std::to_string(character.age) +
        " | Prestige: " + std::to_string(character.prestige) + " | Clan Tension: " + std::to_string(clan.tension) + "/100\n" +
        "Ambition: " + character.ambition.getName() + "\n\n" +
        "--- ATTRIBUTES (Base + Traits) ---\n" +
        "Prowess:      " + std::to_string(stats.prowess) + "\n" +
        "Martial:      " + std::to_string(stats.martial) + "\n" +
        "Stewardship:  " + std::to_string(stats.stewardship) + "\n" +
        "Intrigue:     " + std::to_string(stats.intrigue) + "\n" +
        "Diplomacy:    " + std::to_string(stats.diplomacy) + "\n\n" +
        "--- OPINION OF CURRENT ALPHA ---\n";

    const OpinionMatrix* opMatrix = character.getOpinionBreakdown(alphaId);
    if (character.id == alphaId) {
        info += "* Self-Governing Alpha (+100)\n";
    } else if (opMatrix && !opMatrix->modifiers.empty()) {
        for (const auto& mod : opMatrix->modifiers) {
            info += "* " + mod.reason + ": " + (mod.value >= 0 ? "+" : "") + std::to_string(mod.value) + "\n";
        }
        info += "Total Opinion Score: " + std::to_string(opMatrix->calculateTotal()) + " / 100\n";
    } else {
        info += "* Neutral Kinship Baseline: 0\n";
    }

    info += "\n--- ACTIVE TRAITS ---\n";
    for (TraitID trait : character.traits) {
        switch (trait) {
            case TraitID::SILVERBACK: info += "* Silverback (+Prowess, +Martial, Clan Respect)\n"; break;
            case TraitID::FIERCE_ROAR: info += "* Fierce Roar (+Combat Morale, +Intimidation)\n"; break;
            case TraitID::SNEAKY_FORAGER: info += "* Sneaky Forager (+Intrigue, +Foraging, -Trust)\n"; break;
            case TraitID::WISE_ELDER: info += "* Wise Elder (+Diplomacy, +Clan Stability)\n"; break;
            case TraitID::AMBITIOUS: info += "* Ambitious (+Intrigue, Faction Organizer)\n"; break;
            case TraitID::LOYAL: info += "* Loyal (Plot Resistant, +Opinion)\n"; break;
            case TraitID::COWARD: info += "* Coward (-Prowess, Flee-prone)\n"; break;
            case TraitID::NATURAL_LEADER: info += "* Natural Leader (+Influence, +Recruitment)\n"; break;
            case TraitID::FICKLE_GROOMER: info += "* Fickle Groomer (Unstable Loyalty)\n"; break;
        }
    }

    info += "\n[Left / Right Arrow] Cycle Clan Characters";

    detailText.setString(info);
    detailText.setPosition(startX, startY + 35.f);
    target.draw(detailText);
}

void DynastyUI::drawFamilyTreeView(
    sf::RenderTarget& target,
    const Dynasty& dynasty,
    const std::unordered_map<Character::ID, Character>& registry,
    Character::ID currentAlphaId
) {
    sf::Vector2f center = panelBg.getPosition();
    float startX = center.x - panelBg.getSize().x * 0.46f;
    float startY = center.y - panelBg.getSize().y * 0.45f;

    headerText.setString("GENEALOGICAL LINEAGE: " + dynasty.name + " Dynasty");
    headerText.setPosition(startX, startY);
    target.draw(headerText);

    std::string tree = "Multi-Generational Bloodline Graph:\n\n";

    if (registry.count(currentAlphaId)) {
        const Character& alpha = registry.at(currentAlphaId);
        tree += "[CURRENT ALPHA] " + alpha.name + " (Age " + std::to_string(alpha.age) + ")\n";

        if (alpha.fatherId != Character::INVALID_ID && registry.count(alpha.fatherId)) {
            const Character& dad = registry.at(alpha.fatherId);
            tree += "  |-- Ancestor (Father): " + dad.name + (dad.isAlive ? " [ALIVE]" : " [DEAD]") + "\n";
        }
        if (alpha.motherId != Character::INVALID_ID && registry.count(alpha.motherId)) {
            const Character& mom = registry.at(alpha.motherId);
            tree += "  |-- Ancestor (Mother): " + mom.name + (mom.isAlive ? " [ALIVE]" : " [DEAD]") + "\n";
        }

        tree += "  |-- Siblings:\n";
        for (Character::ID mid : dynasty.memberIds) {
            if (mid != alpha.id && registry.count(mid)) {
                const Character& sib = registry.at(mid);
                if (sib.fatherId == alpha.fatherId && sib.fatherId != Character::INVALID_ID) {
                    tree += "  |    * " + sib.name + " (Age " + std::to_string(sib.age) + ") " + (sib.isAlive ? "[ALIVE]" : "[DEAD]") + "\n";
                }
            }
        }

        tree += "  |-- Descendants (Children):\n";
        for (Character::ID childId : alpha.childrenIds) {
            if (registry.count(childId)) {
                const Character& child = registry.at(childId);
                tree += "  |    * " + child.name + " (Age " + std::to_string(child.age) + ") " + (child.isAlive ? "[ALIVE]" : "[DEAD]") + "\n";
            }
        }
    }

    detailText.setString(tree);
    detailText.setPosition(startX, startY + 35.f);
    target.draw(detailText);
}

void DynastyUI::drawSuccessionView(
    sf::RenderTarget& target,
    const Dynasty& dynasty,
    const Clan& clan,
    const std::unordered_map<Character::ID, Character>& registry,
    const std::vector<Faction>& factions
) {
    sf::Vector2f center = panelBg.getPosition();
    float startX = center.x - panelBg.getSize().x * 0.46f;
    float startY = center.y - panelBg.getSize().y * 0.45f;

    std::string lawName;
    switch (clan.successionLaw) {
        case SuccessionLaw::BLOODLINE_PRIMOGENITURE: lawName = "Bloodline Primogeniture"; break;
        case SuccessionLaw::ELDER_SENIORITY: lawName = "Elder Seniority"; break;
        case SuccessionLaw::RIGHT_OF_THE_STRONGEST: lawName = "Right of the Strongest"; break;
    }

    headerText.setString("SUCCESSION LINE & POLITICAL FACTIONS: " + lawName);
    headerText.setPosition(startX, startY);
    target.draw(headerText);

    auto candidates = SuccessionSystem::evaluateSuccession(dynasty, registry, factions, clan.successionLaw);

    std::string list = "--- RANKED SUCCESSION CANDIDATES ---\n";
    int rank = 1;
    for (const auto& c : candidates) {
        if (!registry.count(c.characterId)) continue;
        const Character& ape = registry.at(c.characterId);

        list += std::to_string(rank) + ". " + ape.name + " | Score: " + std::to_string(static_cast<int>(c.score)) +
                " | " + c.rationale;
        if (c.factionBackingPower > 0.0f) {
            list += " (Backed by Faction: +" + std::to_string(static_cast<int>(c.factionBackingPower)) + " Power)";
        }
        list += "\n";
        rank++;
    }

    list += "\n--- ACTIVE CLAN FACTIONS ---\n";
    if (factions.empty()) {
        list += "No active dissident factions. The clan is united under the Alpha.\n";
    } else {
        for (const auto& f : factions) {
            std::string leaderName = registry.count(f.leaderId) ? registry.at(f.leaderId).name : "Unknown";
            list += "* " + f.name + " | Leader: " + leaderName + " | Members: " + std::to_string(f.memberIds.size()) +
                    " | Military Power: " + std::to_string(static_cast<int>(f.powerRating)) + "\n";
        }
    }

    detailText.setString(list);
    detailText.setPosition(startX, startY + 35.f);
    target.draw(detailText);
}

}