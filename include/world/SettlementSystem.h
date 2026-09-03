#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "simulation/SimulationRegistry.h"

struct RealSettlement {
    sim::VillageID villageId = 0;
    sim::KingdomID kingdomId = 0;
    float borderLeftX = 0.f;
    float borderRightX = 0.f;
    float centerX = 0.f;
    std::string historicalName;
    std::string modernName;
    std::string kingdomName;
    bool isAllied = true;
    sf::Vector2f mapCoord;
};

class SettlementSystem {
private:
    std::vector<RealSettlement> realSettlements;
    int activeSettlementIdx = -1;
    int lastSettlementIdx = -1;

    sf::Font font;
    bool fontLoaded = false;

    float bannerTimer = 0.f;
    bool showBanner = false;
    bool isExiting = false;

    std::string bannerOldName;
    std::string bannerModernName;
    std::string bannerKingdom;
    bool bannerAllied = true;

    float minExploredX = 0.f;
    float maxExploredX = 0.f;
    bool hasExplored = false;

    sf::RectangleShape mapOuterVellum;
    sf::RectangleShape mapInnerVellum;
    sf::RectangleShape mapInnerBorder;
    sf::ConvexShape britainCoast;
    sf::ConvexShape irelandCoast;
    sf::ConvexShape frankiaCoast;
    sf::ConvexShape scandiCoast;
    std::vector<sf::Vertex> rhumbLines;
    std::vector<sf::Vertex> seaWaves;

    float pulseTime = 0.f;

    void buildAuthenticMapGeometry();
    void syncDynamicVillages(sim::SimulationRegistry& registry);
    

public:
    SettlementSystem();

    void syncWithWorld(sim::SimulationRegistry& registry);
    void update(float dt, float playerX, sim::SimulationRegistry& registry);
    void draw(sf::RenderWindow& window, const sf::View& letterboxView);
    void drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX);
    void drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry);
    const RealSettlement* getSettlementAt(float x) const;
    const RealSettlement* getSettlementByVillageId(sim::VillageID id) const;
    bool canFreelyPass(float x) const;
    const RealSettlement* getActiveSettlement() const;
};