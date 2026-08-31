#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include "simulation/SimulationRegistry.h"

class WorldManager;

class StructureManager {
private:
    const sf::Texture* villageTexture = nullptr;

    const sf::IntRect rectCenterBuilding   = sf::IntRect(58,   304, 771, 683);
    const sf::IntRect rectBorderMonument   = sf::IntRect(905,  319, 224, 666);
    const sf::IntRect rectLookpost         = sf::IntRect(1238, 326, 275, 659);
    const sf::IntRect rectLookpostBamboo   = sf::IntRect(2400, 220, 300, 765);
    const sf::IntRect rectToolRack         = sf::IntRect(1640, 330, 340, 215);
    const sf::IntRect rectFirePit          = sf::IntRect(2075, 405, 245, 140);
    const sf::IntRect rectVillageHut       = sf::IntRect(2420, 740, 340, 245);
    const sf::IntRect rectMeetingRootLog   = sf::IntRect(1515, 830, 325, 155);
    const sf::IntRect rectMeetingStone     = sf::IntRect(1860, 830, 210, 155);
    const sf::IntRect rectMeetingHollowLog = sf::IntRect(2085, 875, 290, 110);
    const sf::IntRect rectPalisadeMiddle   = sf::IntRect(40,   1220, 175, 235);
    const sf::IntRect rectPalisadeLeft     = sf::IntRect(255,  1220, 340, 235);
    const sf::IntRect rectPalisadeRight    = sf::IntRect(650,  1220, 175, 235);
    const sf::IntRect rectGateProfile      = sf::IntRect(955,  1210, 110, 245);
    const sf::IntRect rectGateFront        = sf::IntRect(1140, 1195, 380, 260);
    const sf::IntRect rectBambooNode       = sf::IntRect(1970, 1220, 225, 235);
    const sf::IntRect rectFxFire           = sf::IntRect(2300, 1366, 70,  90);
    const sf::IntRect rectFxSmoke          = sf::IntRect(2417, 1364, 68,  91);
    const sf::IntRect rectFxSoul           = sf::IntRect(2543, 1363, 66,  92);
    const sf::IntRect rectFxLeaf           = sf::IntRect(2671, 1367, 76,  85);

    void drawSpriteAnchored(sf::RenderTarget& target, const sf::IntRect& rect, float x, float y, float scale = 0.35f, sf::Color color = sf::Color::White);

public:
    StructureManager();
    void setTexture(const sf::Texture& tex);
    const sf::Texture* getTexture() const { return villageTexture; }

    void update(float dt, sim::SimulationRegistry& registry);
    void draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds);
    void drawForeground(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds);

    void drawSettlementFootprint(sf::RenderTarget& target, const sim::VillageData& village, float groundY);
    void drawVillageCenter(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY);
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