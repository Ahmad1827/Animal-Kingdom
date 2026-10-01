#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "simulation/SimulationRegistry.h"

enum class MapLens {
    DeFacto = 0,
    DeJure = 1,
    Vassals = 2,
    Diplomacy = 3,
    Economy = 4
};

struct CountyDef {
    int countyId = 0;
    sim::VillageID villageId = 0;
    sim::KingdomID kingdomId = 0;
    std::string countyName;
    std::string settlementName;
    std::string modernName;
    std::string kingdomName;
    std::string deJureKingdom;
    std::vector<sf::Vector2f> points;
    sf::ConvexShape shape;
    sf::Vector2f center;
    float siegeProgress = 0.f;
    bool isOccupied = false;
    std::string occupierKingdom;
    int vassalOpinion = 20;
    bool inFaction = false;
    int supplyLimit = 30;
    int fortTier = 0;
    bool leviesRaised = false;
};

struct FactionState {
    std::string name = "Independence League";
    float discontent = 0.f;
    float powerRatio = 0.f;
    std::vector<std::string> memberCounties;
};

struct SuccessionPartitionEntry {
    std::string countyName;
    std::string heirName;
    std::string titleType;
    std::string status;
    sf::Color statusColor;
};

struct TradeRouteNode {
    std::string name;
    std::vector<sf::Vector2f> waypoints;
    std::vector<std::string> counties;
    float baseToll = 3.0f;
    bool isRaided = false;
    std::string raiderKingdom;
};

struct MapArmy {
    uint32_t id = 0;
    std::string ownerKingdom;
    int strength = 20;
    sf::Vector2f pos;
    sf::Vector2f targetPos;
    std::string currentCounty;
    std::string targetCounty;
    std::string originCounty;
    bool isMoving = false;
    bool inCombat = false;
    float combatTimer = 0.f;
    float supply = 100.f;
    float attritionTimer = 0.f;
    bool sufferingAttrition = false;
};

enum class CouncilMissionType {
    None,
    FabricateClaim,
    TrainLevies,
    DevelopCounty
};

struct CouncilAssignment {
    sim::CouncilRole role = sim::CouncilRole::None;
    CouncilMissionType mission = CouncilMissionType::None;
    std::string targetCounty;
    float progress = 0.f;
    float maxProgress = 100.f;
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
    std::string deJureKingdom;
    std::string countyName;
    bool isAllied = true;
    bool hasPlayerClaim = false;
    sf::Vector2f mapCoord;
};

struct ActiveWarState {
    bool active = false;
    std::string targetCounty;
    std::string enemyKingdom;
    std::string attackerKingdom;
    std::string casusBelli;
    float warScore = 65.f;
    float warTimer = 0.f;
    float aiThinkTimer = 0.f;
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

    MapLens currentLens = MapLens::DeFacto;
    sf::FloatRect lensTabBounds[5];

    int targetMapMode = 0;
    float miniAnimT = 0.f;
    float expandAnimT = 0.f;

    static SettlementSystem* s_instance;
    ActiveWarState activeWar;
    bool peaceModalOpen = false;
    sf::FloatRect warBadgeBounds;
    std::vector<MapArmy> mapArmies;
    int selectedArmyId = -1;
    std::vector<CouncilAssignment> councilAssignments;
    std::vector<TradeRouteNode> tradeRoutes;
    void initTradeRoutes();
    std::vector<std::string> activeWarAllies;
    sf::FloatRect callAllyBtnBounds;
    std::string callAllyStatusMsg;
    float callAllyStatusTimer = 0.f;

    int crownAuthority = 2;
    int selectedAuthorityTier = 2;
    float lawCooldownTimer = 0.f;
    std::string lawStatusMsg;
    float lawStatusTimer = 0.f;
    sf::FloatRect authorityTierBounds[4];
    sf::FloatRect enactLawBtnBounds;
    sim::SimulationRegistry* cachedRegistry = nullptr;

    FactionState independenceFaction;
    bool factionModalOpen = false;
    sf::FloatRect factionsTabBounds;
    sf::FloatRect closeFactionModalBounds;
    bool successionModalOpen = false;
    std::string deceasedKingTitle = "King Cynewulf";
    std::string successorTitle = "Ecgberht I";
    std::vector<SuccessionPartitionEntry> successionEntries;
    sf::FloatRect closeSuccessionModalBounds;
    sf::FloatRect confirmSuccessionBtnBounds;
    float shortReignTimer = 0.f;
    sf::FloatRect enforceBtnBounds;
    sf::FloatRect whitePeaceBtnBounds;
    sf::FloatRect surrenderBtnBounds;
    sf::FloatRect closePeaceModalBounds;
    std::unordered_map<std::string, int> kingdomTruces;

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

    static std::unordered_set<std::string>& getPlayerClaims() {
        static std::unordered_set<std::string> claims;
        return claims;
    }

    static SettlementSystem* getInstance();
    static void startWar(const std::string& county, const std::string& attacker, const std::string& enemy, const std::string& cb);
    static void annexCounty(const std::string& county, const std::string& newKingdom);
    static bool isAtWarWith(const std::string& kingdom);
    static bool hasTruceWith(const std::string& kingdom);
    static void spawnArmy(const std::string& county, const std::string& kingdom, int strength);
    static void disbandArmyInCounty(const std::string& county, const std::string& kingdom);
    static bool hasArmyInCounty(const std::string& county, const std::string& kingdom);
    static void assignCouncilMission(sim::CouncilRole role, CouncilMissionType mission, const std::string& county);
    static const std::vector<CouncilAssignment>& getAllCouncilAssignments();
    static const CouncilAssignment* getCouncilAssignment(sim::CouncilRole role);

    static void swayVassal(const std::string& county, int delta);
    static int getVassalOpinion(const std::string& county);
    static bool isCountyInFaction(const std::string& county);
    static void triggerCivilWar(const std::string& county);
    static const FactionState& getFactionState();
    static int getCountySupplyLimit(const std::string& county);
    static int getCountyTroops(const std::string& county);
    static void triggerSuccession();
    static int getCountyFortTier(const std::string& county);
    static void upgradeCountyFort(const std::string& county);

    static const std::vector<TradeRouteNode>& getTradeRoutes();
    static bool isCountyOnTradeRoute(const std::string& county);
    static bool isTradeRouteRaided(const std::string& routeName);
    static void setTradeRouteRaided(const std::string& routeName, bool raided, const std::string& raider);
    static float getKingdomTradeIncome(const std::string& kingdom);
    static float getKingdomArmyUpkeep(const std::string& kingdom);
    static int getKingdomRaisedTroops(const std::string& kingdom);
    static int getKingdomDemesneCount(const std::string& kingdom);
    static bool isWarActive();
    static void callAllyToWar(const std::string& allyKingdom);
    static bool isAllyInWar(const std::string& allyKingdom);

    static int getCrownAuthority();
    static void setCrownAuthority(int level);
    static float getAuthorityTaxMultiplier();
    static int getAuthorityLevyPerCounty();
    static int getAuthorityVassalOpinionMod();

    static bool isCountyLevyRaised(const std::string& county);
    static bool canMusterCountyLevies(const std::string& county, const std::string& kingdom);
    static bool musterCountyLevies(const std::string& county, const std::string& kingdom);

    sf::Vector2f getPlayerMapCoord(float playerX) const;
    float getWestCoastLimit() const { return westCoastX; }
    float getEastCoastLimit() const { return eastCoastX; }
    const RealSettlement* getActiveSettlement() const;
    const RealSettlement* getSettlementAt(float x) const;
    const RealSettlement* getSettlementByVillageId(sim::VillageID id) const;
    bool canFreelyPass(float x) const;
};