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

struct CountyDef {
    int countyId = 0;
    sim::VillageID villageId = 0;
    sim::KingdomID kingdomId = 0;
    std::string countyName;
    std::string settlementName;
    std::string modernName;
    std::string kingdomName;
    std::vector<sf::Vector2f> points;
    sf::ConvexShape shape;
    sf::Vector2f center;
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
    std::string countyName;
    bool isAllied = true;
    sf::Vector2f mapCoord;
};

class SettlementSystem {
private:
    std::vector<RealSettlement> realSettlements;
    std::vector<CountyDef> counties;
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

    sf::ConvexShape frankiaCoast;
    sf::ConvexShape scandiCoast;
    std::vector<sf::Vertex> rhumbLines;
    std::vector<sf::Vertex> seaWaves;

    sf::RectangleShape miniFrameOuter;
    sf::RectangleShape miniFrameInner;
    sf::RectangleShape miniSea;

    sf::RenderTexture mapCanvas;
    bool mapCanvasReady = false;
    sf::Vector2f mapCenter = sf::Vector2f(440.f, 380.f);
    float mapZoom = 1.0f;
    bool isDraggingMap = false;
    sf::Vector2i lastDragMouse;
    sf::Vector2i dragStartMouse;

    int hoveredCountyIdx = -1;
    std::string hoveredKingdomName = "";

    float pulseTime = 0.f;
    float westCoastX = -32800.f;
    float eastCoastX = 368000.f;

    MapLens currentLens = MapLens::Realms;
    sf::FloatRect lensTabBounds[4];

    int targetMapMode = 0;
    float miniAnimT = 0.f;
    float expandAnimT = 0.f;

    void buildAuthenticMapGeometry();
    void buildOrganicCounties();
    void syncDynamicVillages(sim::SimulationRegistry& registry);
    bool pointInPolygon(const std::vector<sf::Vector2f>& poly, sf::Vector2f pt) const;

public:
    SettlementSystem();

    void syncWithWorld(sim::SimulationRegistry& registry);
    void update(float dt, float playerX, sim::SimulationRegistry& registry);
    void draw(sf::RenderWindow& window, const sf::View& letterboxView);
    void drawMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry);
    void drawMinimap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry);
    void drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX);
    void drawWorldMap(sf::RenderWindow& window, const sf::View& letterboxView, float playerX, const sim::SimulationRegistry& registry);
    void drawCoast(sf::RenderTarget& rt, const sf::FloatRect& viewBounds, float groundY, float timeOfDay, const sf::Texture* skyTex = nullptr, const sf::View* cameraView = nullptr);

    bool handleMapLensInput(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView);
    bool handleWorldMapInput(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView,
                             const std::function<void(const RealSettlement&, bool isKingdomLevel)>& onSettlementClicked,
                             const std::function<void(const RealSettlement&, sf::Vector2f, bool isKingdomLevel)>& onSettlementRightClicked = nullptr);

    void setMapMode(int mode);
    bool isExpandedInteractive() const { return expandAnimT > 0.80f; }

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