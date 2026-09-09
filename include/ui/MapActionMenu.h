#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "world/SettlementSystem.h"
#include "simulation/SimulationRegistry.h"

struct MapActionOption {
    std::string title;
    std::string subtitle;
    std::string costText;
    bool isEnabled;
    sf::Color accentColor;
    std::function<void()> onExecute;
};

class MapActionMenu {
private:
    const sf::Font* font;
    bool active;
    sf::Vector2f position;
    float menuWidth;
    float menuHeight;

    RealSettlement targetSettlement;
    bool isKingdomLevel;
    std::vector<MapActionOption> options;
    std::vector<sf::FloatRect> optionBounds;
    int hoveredIdx;

    std::string statusMessage;
    sf::Color statusColor;
    float statusTimer;

    void rebuildOptions(sim::SimulationRegistry& reg, sim::EntityID playerApeId);

public:
    MapActionMenu();

    void init(const sf::Font& f);
    void open(const RealSettlement& rs, sf::Vector2f screenPos, sim::SimulationRegistry& reg, sim::EntityID playerApeId, bool kingdomLevel = false);
    void close();

    void update(float dt);
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView);
    void draw(sf::RenderWindow& window);

    bool isOpen() const { return active; }
};