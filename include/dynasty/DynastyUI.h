// include/dynasty/DynastyUI.h
#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include "dynasty/Character.h"
#include "dynasty/Dynasty.h"
#include "dynasty/Clan.h"

enum class DynastyUIMode {
    CLOSED,
    CHARACTER_VIEW,
    FAMILY_TREE_VIEW,
    SUCCESSION_VIEW
};

class DynastyUI {
public:
    DynastyUI();
    void init(const sf::Font& font);
    void toggle(DynastyUIMode mode);
    bool isOpen() const { return currentMode != DynastyUIMode::CLOSED; }
    void render(
        sf::RenderTarget& target,
        const Dynasty& dynasty,
        const Clan& clan,
        const std::unordered_map<Character::ID, Character>& registry,
        Character::ID currentAlphaId
    );

private:
    DynastyUIMode currentMode = DynastyUIMode::CLOSED;
    sf::Font font;
    sf::RectangleShape panelBg;
    sf::Text headerText;
    sf::Text detailText;

    void drawCharacterView(sf::RenderTarget& target, const Character& character, const Dynasty& dynasty, const Clan& clan);
    void drawFamilyTreeView(sf::RenderTarget& target, const Dynasty& dynasty, const std::unordered_map<Character::ID, Character>& registry, Character::ID currentAlphaId);
    void drawSuccessionView(sf::RenderTarget& target, const Dynasty& dynasty, const Clan& clan, const std::unordered_map<Character::ID, Character>& registry);
};