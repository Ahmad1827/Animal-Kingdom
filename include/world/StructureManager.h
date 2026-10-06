#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include "simulation/SimulationRegistry.h"
#include "simulation/VillageData.h"

class WorldManager;

enum class VillageUpgradePhase {
    Idle,
    Demolishing,
    WaitingForBuilder,
    Building,
    Finishing
};

struct BuildingTierVisual {
    std::string textureKey;
    sf::IntRect spriteRect;
    float scale;
};

class StructureManager {
private:
    const sf::Texture* villageTexture = nullptr;

    // Cells of assets/sprites/medieval/village.png, as printed by tools/build_village.py.
    // Every piece is drawn at scale 1, standing on its bottom edge.
    const sf::IntRect rectHallTimber       = sf::IntRect(1014, 2, 750, 510);
    const sf::IntRect rectHallStone        = sf::IntRect(2, 2, 768, 648);
    const sf::IntRect rectLookpostBamboo   = sf::IntRect(2, 652, 228, 300);
    const sf::IntRect rectBorderMonument   = sf::IntRect(1766, 2, 120, 324);
    const sf::IntRect rectToolRack         = sf::IntRect(232, 652, 312, 216);
    const sf::IntRect rectVillageHut       = sf::IntRect(546, 652, 228, 168);
    const sf::IntRect rectFirePit          = sf::IntRect(1424, 652, 216, 108);
    const sf::IntRect rectBrazier          = sf::IntRect(1642, 652, 48, 108);
    const sf::IntRect rectFxFire           = sf::IntRect(1692, 652, 72, 90);
    const sf::IntRect rectMeetingHollowLog = sf::IntRect(1170, 652, 252, 114);
    const sf::IntRect rectMeetingStone     = sf::IntRect(970, 652, 198, 120);
    const sf::IntRect rectPalisadeMiddle   = sf::IntRect(776, 652, 192, 138);
    const sf::IntRect rectPalisadeRear     = sf::IntRect(1766, 652, 180, 75);

    const sf::Texture* groundTexture = nullptr;
    const sf::Texture* rearLawnTexture = nullptr;
    float shadowShearX = 0.f;
    float shadowProjY = 0.2f;
    sf::Color shadowColor = sf::Color(10, 14, 22, 100);
    bool enableShadows = true;

    void drawCastle(sf::RenderTarget& target, const sim::VillageData& village, float groundY);
    void drawSpriteAnchored(sf::RenderTarget& target, const sf::IntRect& rect, float x, float y, float scale, sf::Color color = sf::Color::White);

    VillageUpgradePhase upgradePhase = VillageUpgradePhase::Idle;
    float upgradeTimer = 0.f;
    float buildProgress = 0.f;
    const float totalBuildDuration = 12.0f;
    int upgradeCost = 8;
    sim::EntityID activeBuilderId = 0;

    bool upgradeModalOpen = false;
    sf::FloatRect modalUpgradeButtonBounds;
    sf::FloatRect modalCloseButtonBounds;

    BuildingTierVisual tier1Visual = { "village_assets", sf::IntRect(1014, 2, 750, 510), 1.0f };
    BuildingTierVisual tier2Visual = { "village_assets", sf::IntRect(2, 2, 768, 648), 1.0f };

public:
    StructureManager();
    void setTexture(const sf::Texture& tex);
    const sf::Texture* getTexture() const { return villageTexture; }

    void update(float dt, sim::SimulationRegistry& registry);
    void setShadowParams(float shearX, float projY, sf::Color color);
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
    void setGroundTexture(const sf::Texture& tex);
    void setRearLawnTexture(const sf::Texture& tex);

    void setTier2Visual(const std::string& textureKey, const sf::IntRect& rect, float scale = 1.0f);
    void updateUpgrades(float dt, sim::SimulationRegistry& registry);
    bool tryStartUpgrade(sim::VillageData& village, sim::SimulationRegistry& registry);
    bool isUpgrading() const { return upgradePhase != VillageUpgradePhase::Idle; }
    int getUpgradeCost() const { return upgradeCost; }

    void openUpgradeModal() { upgradeModalOpen = true; }
    void closeUpgradeModal() { upgradeModalOpen = false; }
    bool isUpgradeModalOpen() const { return upgradeModalOpen; }
    bool handleModalClick(const sf::Vector2f& uiCoords, sim::VillageData& village, sim::SimulationRegistry& registry);
    void drawUpgradeModal(sf::RenderTarget& target, const sf::Font& font, int villageAmber, sim::SettlementTier tier);
};