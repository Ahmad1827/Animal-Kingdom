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
    bool isInitialized = false;

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

    sf::RectangleShape miniFrameOuter;
    sf::RectangleShape miniFrameInner;
    sf::RectangleShape miniSea;
    sf::ConvexShape miniBritain;
    sf::ConvexShape miniIreland;

    float pulseTime = 0.f;
    float westCoastX = -20800.f;
    float eastCoastX = 220000.f;

    void buildAuthenticMapGeometry();
    void syncDynamicVillages(sim::SimulationRegistry& registry);

public:
    SettlementSystem();

    void syncWithWorld(sim::SimulationRegistry& registry);
    void update(float dt, float playerX, sim::SimulationRegistry& registry);
    void draw(sf::RenderWindow& window, const sf::View& letterboxView);
    void drawMinimap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry);
    void drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX);
    void drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry);
    void drawCoast(sf::RenderTarget& rt, const sf::FloatRect& viewBounds, float groundY, float timeOfDay, const sf::Texture* skyTex = nullptr, const sf::View* cameraView = nullptr);

    sf::Vector2f getPlayerMapCoord(float playerX) const;
    float getWestCoastLimit() const { return westCoastX; }
    float getEastCoastLimit() const { return eastCoastX; }
    const RealSettlement* getActiveSettlement() const;
    const RealSettlement* getSettlementAt(float x) const;
    const RealSettlement* getSettlementByVillageId(sim::VillageID id) const;
    bool canFreelyPass(float x) const;
};