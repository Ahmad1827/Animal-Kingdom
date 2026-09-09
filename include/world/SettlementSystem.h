#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "simulation/SimulationRegistry.h"

enum class MapLens {
    Realms = 0,
    Diplomacy = 1,
    Tension = 2,
    Economy = 3
};

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
    int alignX = 1;
    int alignY = 0;
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

    sf::ConvexShape cornwallShape;
    sf::ConvexShape wessexShape;
    sf::ConvexShape eastAngliaShape;
    sf::ConvexShape merciaShape;
    sf::ConvexShape northumbriaShape;
    sf::ConvexShape albaShape;
    sf::ConvexShape irelandShape;
    sf::ConvexShape frankiaCoast;
    sf::ConvexShape scandiCoast;

    std::vector<sf::Vertex> rhumbLines;
    std::vector<sf::Vertex> seaWaves;

    sf::RectangleShape miniFrameOuter;
    sf::RectangleShape miniFrameInner;
    sf::RectangleShape miniSea;
    sf::ConvexShape miniBritain;
    sf::ConvexShape miniIreland;

    sf::RenderTexture mapCanvas;
    bool mapCanvasReady = false;
    sf::Vector2f mapCenter = sf::Vector2f(440.f, 380.f);
    float mapZoom = 1.0f;
    bool isDraggingMap = false;
    sf::Vector2i lastDragMouse;
    sf::Vector2i dragStartMouse;

    float pulseTime = 0.f;
    float westCoastX = -32800.f;
    float eastCoastX = 368000.f;

    MapLens currentLens = MapLens::Realms;
    sf::FloatRect lensTabBounds[4];

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

    bool handleMapLensInput(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView);
    bool handleWorldMapInput(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, const std::function<void(const RealSettlement&)>& onSettlementClicked);

    void setMapLens(MapLens lens) { currentLens = lens; }
    MapLens getMapLens() const { return currentLens; }

    sf::Vector2f getPlayerMapCoord(float playerX) const;
    float getWestCoastLimit() const { return westCoastX; }
    float getEastCoastLimit() const { return eastCoastX; }
    const RealSettlement* getActiveSettlement() const;
    const RealSettlement* getSettlementAt(float x) const;
    const RealSettlement* getSettlementByVillageId(sim::VillageID id) const;
    bool canFreelyPass(float x) const;
};