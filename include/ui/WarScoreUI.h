#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "simulation/SimulationRegistry.h"

enum class MapLensMode {
    DeFacto = 0,
    DeJure = 1,
    Vassals = 2,
    Diplomacy = 3,
    Economy = 4
};

class WarScoreUI {
private:
    const sf::Font* font = nullptr;
    MapLensMode currentLens = MapLensMode::DeFacto;
    bool peaceModalOpen = false;
    uint32_t activeWarId = 0;
    float pulseTime = 0.f;

    struct LensButton {
        MapLensMode mode;
        std::string label;
        std::string shortcutKey;
        sf::FloatRect bounds;
    };
    std::vector<LensButton> lensButtons;

    sf::FloatRect warBadgeBounds;
    sf::FloatRect enforceBtnBounds;
    sf::FloatRect whitePeaceBtnBounds;
    sf::FloatRect surrenderBtnBounds;
    sf::FloatRect closePeaceBtnBounds;

public:
    WarScoreUI();

    void init(const sf::Font& font);
    void update(float dt);
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);
    void draw(sf::RenderWindow& window, sim::SimulationRegistry& reg, sim::EntityID controlledApeId, bool isExpandedMap);

    MapLensMode getCurrentLens() const { return currentLens; }
    void setLens(MapLensMode lens) { currentLens = lens; }
    bool isPeaceModalOpen() const { return peaceModalOpen; }
};