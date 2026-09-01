#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include "simulation/SimulationRegistry.h"

class WorldManager;

class StructureManager {
private:
    const sf::Texture* villageTexture = nullptr;

    const sf::IntRect rectCenterBuilding   = sf::IntRect(59,   303,  770, 683);
    const sf::IntRect rectBorderMonument   = sf::IntRect(907,  319,  220, 666);
    const sf::IntRect rectLookpost         = sf::IntRect(1237, 325,  276, 660);
    const sf::IntRect rectLookpostBamboo   = sf::IntRect(2419, 221,  285, 389);
    const sf::IntRect rectToolRack         = sf::IntRect(1647, 330,  334, 280);
    const sf::IntRect rectFirePit          = sf::IntRect(2085, 433,  233, 172);
    const sf::IntRect rectVillageHut       = sf::IntRect(2426, 766,  327, 219);
    const sf::IntRect rectMeetingRootLog   = sf::IntRect(1586, 832,  252, 153);
    const sf::IntRect rectMeetingStone     = sf::IntRect(1860, 830,  209, 155);
    const sf::IntRect rectMeetingHollowLog = sf::IntRect(2090, 886,  285, 99);
    const sf::IntRect rectGateProfile      = sf::IntRect(961,  1215, 101, 239);
    const sf::IntRect rectGateFront        = sf::IntRect(1142, 1199, 375, 255);
    const sf::IntRect rectPalisadeMiddle   = sf::IntRect(42,   1225, 173, 229);
    const sf::IntRect rectPalisadeLeft     = sf::IntRect(257,  1224, 332, 230);
    const sf::IntRect rectPalisadeRight    = sf::IntRect(653,  1225, 173, 229);
    const sf::IntRect rectBambooNode       = sf::IntRect(1973, 1225, 221, 233);
    const sf::IntRect rectFxFire           = sf::IntRect(2285, 1382, 85,  76);
    const sf::IntRect rectFxSmoke          = sf::IntRect(2418, 1365, 74,  93);

    void drawSpriteAnchored(sf::RenderTarget& target, const sf::IntRect& rect, float x, float y, float scale, sf::Color color = sf::Color::White);

public:
    StructureManager();
    void setTexture(const sf::Texture& tex);
    const sf::Texture* getTexture() const { return villageTexture; }

    void update(float dt, sim::SimulationRegistry& registry);
    void draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds);
    void drawForeground(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds);

    void drawSettlementFootprint(sf::RenderTarget& target, const sim::VillageData& village, float groundY);
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