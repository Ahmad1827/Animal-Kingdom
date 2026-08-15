#include "dynasty/DynastyUI.h"
#include "dynasty/Succession.h"

namespace sim {

DynastyUI::DynastyUI() {
    panelBg.setFillColor(sf::Color(15, 12, 8, 235));
    panelBg.setOutlineColor(sf::Color(160, 120, 60));
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

void DynastyUI::render(
    sf::RenderTarget& target,
    const Dynasty& dynasty,
    const Clan& clan,
    const std::unordered_map<Character::ID, Character>& registry,
    Character::ID currentAlphaId
) {
    if (currentMode == DynastyUIMode::CLOSED) return;

    sf::Vector2f viewSize = target.getView().getSize();
    panelBg.setSize(sf::Vector2f(viewSize.x * 0.72f, viewSize.y * 0.72f));
    panelBg.setOrigin(panelBg.getSize().x / 2.f, panelBg.getSize().y / 2.f);
    panelBg.setPosition(target.getView().getCenter());

    target.draw(panelBg);

    switch (currentMode) {
        case DynastyUIMode::CHARACTER_VIEW: {
            if (registry.count(currentAlphaId)) {
                drawCharacterView(target, registry.at(currentAlphaId), dynasty, clan);
            }
            break;
        }
        case DynastyUIMode::FAMILY_TREE_VIEW:
            drawFamilyTreeView(target, dynasty, registry, currentAlphaId);
            break;
        case DynastyUIMode::SUCCESSION_VIEW:
            drawSuccessionView(target, dynasty, clan, registry);
            break;
        default:
            break;
    }
}

void DynastyUI::drawCharacterView(sf::RenderTarget& target, const Character& character, const Dynasty& dynasty, const Clan& clan) {
    sf::Vector2f center = panelBg.getPosition();
    float startX = center.x - panelBg.getSize().x * 0.44f;
    float startY = center.y - panelBg.getSize().y * 0.42f;

    headerText.setString("ALPHA PROFILE: " + character.name + " (" + dynasty.name + ")");
    headerText.setPosition(startX, startY);
    target.draw(headerText);

    CharacterStats stats = character.getEffectiveStats();
    std::string info = 
        "Status: " + std::string(character.isAlive ? "ALIVE" : "DEAD") + "\n" +
        "Age: " + std::to_string(character.age) + "\n" +
        "Clan Tension: " + std::to_string(clan.tension) + " / 100\n\n" +
        "--- ATTRIBUTES ---\n" +
        "Prowess:      " + std::to_string(stats.prowess) + "\n" +
        "Martial:      " + std::to_string(stats.martial) + "\n" +
        "Stewardship:  " + std::to_string(stats.stewardship) + "\n" +
        "Intrigue:     " + std::to_string(stats.intrigue) + "\n" +
        "Diplomacy:    " + std::to_string(stats.diplomacy) + "\n\n" +
        "--- TRAITS ---\n";

    for (TraitID trait : character.traits) {
        switch (trait) {
            case TraitID::SILVERBACK: info += "* Silverback (+Prowess, +Martial)\n"; break;
            case TraitID::FIERCE_ROAR: info += "* Fierce Roar (+Combat Morale)\n"; break;
            case TraitID::SNEAKY_FORAGER: info += "* Sneaky Forager (+Intrigue, +Food)\n"; break;
            case TraitID::WISE_ELDER: info += "* Wise Elder (+Diplomacy)\n"; break;
            case TraitID::AMBITIOUS: info += "* Ambitious (+Intrigue)\n"; break;
            case TraitID::LOYAL: info += "* Loyal (+Stability)\n"; break;
            case TraitID::COWARD: info += "* Coward (-Prowess)\n"; break;
            case TraitID::NATURAL_LEADER: info += "* Natural Leader (+Influence)\n"; break;
            case TraitID::FICKLE_GROOMER: info += "* Fickle Groomer\n"; break;
        }
    }

    detailText.setString(info);
    detailText.setPosition(startX, startY + 45.f);
    target.draw(detailText);
}

void DynastyUI::drawFamilyTreeView(
    sf::RenderTarget& target,
    const Dynasty& dynasty,
    const std::unordered_map<Character::ID, Character>& registry,
    Character::ID currentAlphaId
) {
    sf::Vector2f center = panelBg.getPosition();
    float startX = center.x - panelBg.getSize().x * 0.44f;
    float startY = center.y - panelBg.getSize().y * 0.42f;

    headerText.setString("GENEALOGY: " + dynasty.name + " Lineage");
    headerText.setPosition(startX, startY);
    target.draw(headerText);

    std::string tree = "Lineage Trace:\n\n";

    if (registry.count(currentAlphaId)) {
        const Character& alpha = registry.at(currentAlphaId);
        tree += "[CURRENT ALPHA] " + alpha.name + " (Age " + std::to_string(alpha.age) + ")\n";

        if (alpha.fatherId != Character::INVALID_ID && registry.count(alpha.fatherId)) {
            tree += "  |-- Father: " + registry.at(alpha.fatherId).name + " [DEAD]\n";
        }
        if (alpha.motherId != Character::INVALID_ID && registry.count(alpha.motherId)) {
            tree += "  |-- Mother: " + registry.at(alpha.motherId).name + "\n";
        }

        tree += "  |-- Offspring:\n";
        for (Character::ID childId : alpha.childrenIds) {
            if (registry.count(childId)) {
                const Character& child = registry.at(childId);
                tree += "  |    * " + child.name + " (Age " + std::to_string(child.age) + ") " + (child.isAlive ? "[ALIVE]" : "[DEAD]") + "\n";
            }
        }
    }

    detailText.setString(tree);
    detailText.setPosition(startX, startY + 45.f);
    target.draw(detailText);
}

void DynastyUI::drawSuccessionView(
    sf::RenderTarget& target,
    const Dynasty& dynasty,
    const Clan& clan,
    const std::unordered_map<Character::ID, Character>& registry
) {
    sf::Vector2f center = panelBg.getPosition();
    float startX = center.x - panelBg.getSize().x * 0.44f;
    float startY = center.y - panelBg.getSize().y * 0.42f;

    std::string lawName;
    switch (clan.successionLaw) {
        case SuccessionLaw::BLOODLINE_PRIMOGENITURE: lawName = "Bloodline Primogeniture"; break;
        case SuccessionLaw::ELDER_SENIORITY: lawName = "Elder Seniority"; break;
        case SuccessionLaw::RIGHT_OF_THE_STRONGEST: lawName = "Right of the Strongest"; break;
    }

    headerText.setString("SUCCESSION LINE: " + lawName);
    headerText.setPosition(startX, startY);
    target.draw(headerText);

    auto candidates = SuccessionSystem::evaluateSuccession(dynasty, registry, clan.successionLaw);

    std::string list = "Ranked Candidates:\n\n";
    int rank = 1;
    for (const auto& c : candidates) {
        if (!registry.count(c.characterId)) continue;
        const Character& ape = registry.at(c.characterId);

        list += std::to_string(rank) + ". " + ape.name + " | Score: " + std::to_string(static_cast<int>(c.score)) + 
                " | " + c.rationale + "\n";
        rank++;
    }

    detailText.setString(list);
    detailText.setPosition(startX, startY + 45.f);
    target.draw(detailText);
}

}