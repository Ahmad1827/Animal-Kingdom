#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include "dynasty/Character.h"
#include "dynasty/Dynasty.h"
#include "dynasty/Clan.h"
#include "dynasty/Faction.h"

namespace sim {

enum class DynastyUIMode {
    CLOSED,
    CHARACTER_VIEW,
    FAMILY_TREE_VIEW,
    SUCCESSION_VIEW
};

struct TooltipInfo {
    bool active = false;
    std::string title;
    std::string description;
    sf::Vector2f position;
};

class DynastyUI {
public:
    DynastyUI();
    void init(const sf::Font& font);
    void toggle(DynastyUIMode mode);
    void close();
    bool isOpen() const { return currentMode != DynastyUIMode::CLOSED; }
    DynastyUIMode getMode() const { return currentMode; }

    void nextCharacter(const Dynasty& dynasty);
    void previousCharacter(const Dynasty& dynasty);
    void handleMouseMove(sf::Vector2f mousePos);
    bool handleMouseClick(sf::Vector2f mousePos, const Dynasty& dynasty);

    void render(
        sf::RenderTarget& target,
        const Dynasty& dynasty,
        const Clan& clan,
        const std::unordered_map<Character::ID, Character>& registry,
        const std::vector<Faction>& factions,
        Character::ID currentAlphaId
    );

private:
    DynastyUIMode currentMode = DynastyUIMode::CLOSED;
    sf::Font font;
    size_t inspectedMemberIndex = 0;
    sf::Vector2f currentMousePos;
    TooltipInfo activeTooltip;

    // Visual Palette & Geometry Helpers
    void drawParchmentFrame(sf::RenderTarget& target, sf::FloatRect bounds, const std::string& title, const std::string& subtitle);
    void drawSubPanel(sf::RenderTarget& target, sf::FloatRect bounds, const std::string& header);
    void drawNavTabs(sf::RenderTarget& target, sf::FloatRect panelBounds);
    void drawTooltip(sf::RenderTarget& target);

    // Screen Renderers
    void drawCharacterView(
        sf::RenderTarget& target,
        sf::FloatRect bounds,
        const Character& character,
        const Dynasty& dynasty,
        const Clan& clan,
        Character::ID alphaId
    );

    void drawFamilyTreeView(
        sf::RenderTarget& target,
        sf::FloatRect bounds,
        const Dynasty& dynasty,
        const std::unordered_map<Character::ID, Character>& registry,
        Character::ID currentAlphaId
    );

    void drawSuccessionView(
        sf::RenderTarget& target,
        sf::FloatRect bounds,
        const Dynasty& dynasty,
        const Clan& clan,
        const std::unordered_map<Character::ID, Character>& registry,
        const std::vector<Faction>& factions
    );

    void drawCharacterNode(
        sf::RenderTarget& target,
        sf::Vector2f pos,
        const Character& character,
        bool isAlpha,
        bool isCurrentInspected
    );
};

}