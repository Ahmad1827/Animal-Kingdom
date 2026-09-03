#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "simulation/SimulationRegistry.h"

struct SettlementData {
    sim::VillageID villageId;
    float borderLeftX;
    float borderRightX;
    std::string historicalName;
    std::string modernName;
    std::string kingdom;
    bool isAlliedWithWessex;
    sf::Vector2f mapCoord;
};

class SettlementSystem {
private:
    std::vector<SettlementData> settlements;
    int currentSettlementIdx = -1;
    int previousSettlementIdx = -1;

    sf::Font font;
    bool fontLoaded = false;

    float animTimer = 0.f;
    bool isAnimating = false;
    bool isLeaving = false;

    std::string displayOldName;
    std::string displayModernName;
    std::string displayKingdom;
    bool displayAllied = true;

    float minExploredX = -800.f;
    float maxExploredX = 2000.f;

    sf::RectangleShape mapOverlay;
    sf::RectangleShape parchmentBg;
    sf::RectangleShape parchmentBorderOuter;
    sf::RectangleShape parchmentBorderInner;
    std::vector<sf::Vertex> compassRays;
    std::vector<sf::Vertex> coastOutline;
    std::vector<sf::Vertex> channelOutline;
    std::vector<sf::Vertex> scandiOutline;

    void initMapGraphics();

public:
    SettlementSystem();

    void syncWithWorld(sim::SimulationRegistry& registry);
    void update(float dt, float playerX);
    void draw(sf::RenderWindow& window, const sf::View& letterboxView);
    void drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX);

    float getWestCoastLimit() const { return -4200.f; }
    bool canFreelyPass(float playerX) const;
};