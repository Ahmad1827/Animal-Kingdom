#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include "simulation/SimulationRegistry.h"

class WorldManager;

class StructureManager {
private:
    const sf::Texture* villageTexture = nullptr;

    const sf::IntRect rectCenterBuilding   = sf::IntRect(8,    1406, 771, 683);
    const sf::IntRect rectBorderMonument   = sf::IntRect(1070, 1423, 224, 666);
    const sf::IntRect rectLookpost         = sf::IntRect(1832, 1430, 275, 659);
    const sf::IntRect rectLookpostBamboo   = sf::IntRect(1039, 303,  285, 388);
    const sf::IntRect rectToolRack         = sf::IntRect(227,  411,  334, 280);
    const sf::IntRect rectFirePit          = sf::IntRect(278,  1206, 232, 184);
    const sf::IntRect rectVillageHut       = sf::IntRect(1806, 2569, 327, 219);
    const sf::IntRect rectMeetingRootLog   = sf::IntRect(233,  2631, 321, 157);
    const sf::IntRect rectMeetingStone     = sf::IntRect(1080, 2634, 204, 154);
    const sf::IntRect rectMeetingHollowLog = sf::IntRect(251,  3388, 285, 99);
    const sf::IntRect rectGateProfile      = sf::IntRect(2706, 3947, 104, 239);
    const sf::IntRect rectGateFront        = sf::IntRect(3358, 3930, 376, 256);
    const sf::IntRect rectPalisadeLeft     = sf::IntRect(306,  3956, 175, 230);
    const sf::IntRect rectPalisadeMiddle   = sf::IntRect(1014, 3955, 335, 231);
    const sf::IntRect rectPalisadeRight    = sf::IntRect(1883, 3956, 173, 230);
    const sf::IntRect rectBambooNode       = sf::IntRect(4223, 3957, 222, 229);
    const sf::IntRect rectFxFire           = sf::IntRect(359,  4795, 70,  90);
    const sf::IntRect rectFxSmoke          = sf::IntRect(1148, 4794, 68,  91);

    void drawSpriteAnchored(sf::RenderTarget& target, const sf::IntRect& rect, float x, float y, float scale, sf::Color color = sf::Color::White);

public:
    StructureManager();
    void setTexture(const sf::Texture& tex);
    const sf::Texture* getTexture() const { return villageTexture; }

    void update(float dt, sim::SimulationRegistry& registry);

    void draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds);
    void drawBackgroundStructures(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds);
    void drawMidgroundStructures(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds);
    void drawForeground(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds);

    void drawSettlementFootprint(sf::RenderTarget& target, const sim::VillageData& village, float groundY);
    void drawRearLawn(sf::RenderTarget& target, const sim::VillageData& village, float groundY);
    void drawRearPalisade(sf::RenderTarget& target, const sim::VillageData& village, float groundY);
    void drawMiddlePalisade(sf::RenderTarget& target, const sim::VillageData& village, float groundY);
    void drawFrontRoad(sf::RenderTarget& target, const sim::VillageData& village, float groundY);

    void drawVillageCenter(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawThrone(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawToolRack(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawStockpileProps(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawSimpleBarrier(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawNest(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawStorageHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawWatchPlatform(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawBuilderHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawBonfire(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
    void drawMeetingGround(sf::RenderTarget& target, float worldX, float groundY);
    void drawConstructionSite(sf::RenderTarget& target, const sim::StructureData& s, float groundY);
};