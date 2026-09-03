#include "world/StructureManager.h"
#include "world/WorldManager.h"
#include <cmath>
#include <algorithm>

StructureManager::StructureManager() {}

void StructureManager::setTexture(const sf::Texture& tex) {
    villageTexture = &tex;
}

void StructureManager::update(float, sim::SimulationRegistry&) {}

void StructureManager::drawSpriteAnchored(sf::RenderTarget& target, const sf::IntRect& rect, float x, float y, float scale, sf::Color color) {
    if (!villageTexture || villageTexture->getSize().x == 0) return;
    sf::Sprite sprite(*villageTexture, rect);
    sprite.setOrigin(static_cast<float>(rect.width) * 0.5f, static_cast<float>(rect.height));
    sprite.setPosition(x, y);
    sprite.setScale(scale, scale);

    if (enableShadows && rect != rectFxFire && rect != rectFxSmoke) {
        sf::Sprite shadowSpr = sprite;
        shadowSpr.setColor(shadowColor);
        sf::Transform shadowProj(
            1.f, -shadowShearX, shadowShearX * y,
            0.f, -shadowProjY,  (1.f + shadowProjY) * y,
            0.f, 0.f,           1.f
        );
        target.draw(shadowSpr, shadowProj);
    }

    sprite.setColor(color);
    target.draw(sprite);
}

static void drawGroundShadow(sf::RenderTarget& target, float x, float y, float radiusX, float radiusY, sf::Uint8 alpha = 100) {
    sf::CircleShape shadow(radiusX);
    shadow.setScale(1.0f, radiusY / radiusX);
    shadow.setOrigin(radiusX, radiusX);
    shadow.setPosition(x, y);
    shadow.setFillColor(sf::Color(0, 0, 0, alpha));
    target.draw(shadow);
}

static void drawEmptyPlot(sf::RenderTarget& target, const sim::StructureData& s, float groundY) {
    sf::RectangleShape plotBase(sf::Vector2f(130.f, 6.f));
    plotBase.setOrigin(65.f, 3.f);
    plotBase.setPosition(s.worldX, groundY - 2.f);
    plotBase.setFillColor(sf::Color(105, 80, 50, 180));
    target.draw(plotBase);

    for (int i = -1; i <= 1; i += 2) {
        sf::RectangleShape peg(sf::Vector2f(8.f, 36.f));
        peg.setOrigin(4.f, 36.f);
        peg.setPosition(s.worldX + i * 55.f, groundY);
        peg.setFillColor(sf::Color(85, 55, 28));
        peg.setOutlineColor(sf::Color(20, 10, 5));
        peg.setOutlineThickness(1.5f);
        target.draw(peg);

        sf::ConvexShape pennant(3);
        pennant.setPoint(0, sf::Vector2f(0.f, -34.f));
        pennant.setPoint(1, sf::Vector2f(i * 18.f, -27.f));
        pennant.setPoint(2, sf::Vector2f(0.f, -20.f));
        pennant.setPosition(s.worldX + i * 55.f, groundY);
        pennant.setFillColor(sf::Color(220, 180, 60));
        pennant.setOutlineColor(sf::Color(24, 14, 6));
        pennant.setOutlineThickness(1.f);
        target.draw(pennant);
    }

    sf::RectangleShape twine(sf::Vector2f(110.f, 2.f));
    twine.setOrigin(55.f, 1.f);
    twine.setPosition(s.worldX, groundY - 24.f);
    twine.setFillColor(sf::Color(210, 185, 120, 220));
    target.draw(twine);

    sf::CircleShape centerStone(8.f, 5);
    centerStone.setOrigin(8.f, 8.f);
    centerStone.setPosition(s.worldX, groundY - 3.f);
    centerStone.setFillColor(sf::Color(90, 85, 75));
    centerStone.setOutlineColor(sf::Color(25, 20, 15));
    centerStone.setOutlineThickness(1.5f);
    target.draw(centerStone);
}

void StructureManager::drawMeetingGround(sf::RenderTarget& target, float worldX, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectMeetingRootLog, worldX - 320.f, groundY, 0.90f);
        drawSpriteAnchored(target, rectBorderMonument, worldX, groundY, 1.0f);
        drawSpriteAnchored(target, rectMeetingStone, worldX + 160.f, groundY, 0.90f);
        drawSpriteAnchored(target, rectMeetingHollowLog, worldX + 320.f, groundY, 0.90f);
    }
}

void StructureManager::drawRearLawn(sf::RenderTarget& target, const sim::VillageData&, float groundY) {
    sf::View view = target.getView();
    float viewLeft = view.getCenter().x - view.getSize().x * 0.5f - 400.f;
    float viewWidth = view.getSize().x + 800.f;

    float yardTopY = groundY - 228.f;
    float yardBottomY = groundY - 14.f;
    float yardH = yardBottomY - yardTopY;

    if (rearLawnTexture && rearLawnTexture->getSize().x > 0) {
        float texW = static_cast<float>(rearLawnTexture->getSize().x);
        float texH = static_cast<float>(rearLawnTexture->getSize().y);

        float scale = yardH / texH;
        float stepW = std::floor(texW * scale);
        if (stepW <= 0.f) stepW = 100.f;

        float startX = std::floor(viewLeft / stepW) * stepW;
        float endX = viewLeft + viewWidth;

        for (float x = startX; x < endX; x += stepW) {
            sf::Sprite spr(*rearLawnTexture);
            spr.setScale(scale, scale);
            spr.setPosition(x, yardTopY);
            target.draw(spr);
        }
    } else {
        sf::RectangleShape rearLawn(sf::Vector2f(viewWidth, yardH));
        rearLawn.setPosition(viewLeft, yardTopY);
        rearLawn.setFillColor(sf::Color(30, 56, 26));
        target.draw(rearLawn);

        sf::RectangleShape midLawn(sf::Vector2f(viewWidth, yardH * 0.55f));
        midLawn.setPosition(viewLeft, yardTopY + (yardH * 0.45f));
        midLawn.setFillColor(sf::Color(44, 78, 36));
        target.draw(midLawn);

        sf::RectangleShape rearGrassTrim(sf::Vector2f(viewWidth, 6.f));
        rearGrassTrim.setPosition(viewLeft, yardTopY);
        rearGrassTrim.setFillColor(sf::Color(78, 146, 52));
        target.draw(rearGrassTrim);
    }
}

void StructureManager::drawRearPalisade(sf::RenderTarget& target, const sim::VillageData&, float groundY) {
    if (!villageTexture || villageTexture->getSize().x == 0) return;

    sf::View view = target.getView();
    float viewLeft = view.getCenter().x - view.getSize().x * 0.5f - 400.f;
    float viewRight = view.getCenter().x + view.getSize().x * 0.5f + 400.f;

    float rearFenceBaseY = groundY - 223.f;
    float fenceScale = 0.30f;
    float stepW = static_cast<float>(rectPalisadeMiddle.width) * fenceScale * 0.96f;
    sf::Color fenceBgColor(180, 185, 200, 245);

    float startX = std::floor(viewLeft / stepW) * stepW;
    for (float fx = startX; fx < viewRight; fx += stepW) {
        drawSpriteAnchored(target, rectPalisadeMiddle, fx, rearFenceBaseY, fenceScale, fenceBgColor);
    }
}



void StructureManager::drawMiddlePalisade(sf::RenderTarget& target, const sim::VillageData& village, float groundY) {
    if (!villageTexture || villageTexture->getSize().x == 0) return;

    sf::View view = target.getView();
    float viewLeft = view.getCenter().x - view.getSize().x * 0.5f - 400.f;
    float viewRight = view.getCenter().x + view.getSize().x * 0.5f + 400.f;

    float midFenceBaseY = groundY - 14.f;
    float fenceScale = 0.58f;
    float stepW = static_cast<float>(rectPalisadeMiddle.width) * fenceScale * 0.96f;
    sf::Color fenceColor(230, 230, 235, 255);

    float lodgeHalfWidth = 240.f;
    float startX = std::floor(viewLeft / stepW) * stepW;

    for (float fx = startX; fx < viewRight; fx += stepW) {
        // Leave an opening for the Clan Lodge door
        if (fx >= village.centerX - lodgeHalfWidth && fx <= village.centerX + lodgeHalfWidth) {
            continue;
        }
        drawSpriteAnchored(target, rectPalisadeMiddle, fx, midFenceBaseY, fenceScale, fenceColor);
    }
}

void StructureManager::drawFrontRoad(sf::RenderTarget& target, const sim::VillageData&, float groundY) {
    if (!groundTexture || groundTexture->getSize().x == 0) return;

    sf::View view = target.getView();
    float viewLeft = view.getCenter().x - view.getSize().x * 0.5f - 400.f;
    float viewWidth = view.getSize().x + 800.f;

    float texW = static_cast<float>(groundTexture->getSize().x);
    float texH = static_cast<float>(groundTexture->getSize().y);

    float targetRoadH = 160.f;
    float scale = targetRoadH / texH;
    float topY = groundY - 24.f;

    float stepW = std::floor(texW * scale);
    if (stepW <= 0.f) stepW = 100.f;
    float startX = std::floor(viewLeft / stepW) * stepW;
    float endX = viewLeft + viewWidth;

    for (float x = startX; x < endX; x += stepW) {
        sf::Sprite spr(*groundTexture);
        spr.setScale(scale, scale);
        spr.setPosition(x, topY);
        target.draw(spr);
    }
}


void StructureManager::drawSettlementFootprint(sf::RenderTarget& target, const sim::VillageData& village, float groundY) {
    drawRearLawn(target, village, groundY);
    drawRearPalisade(target, village, groundY);
    drawMiddlePalisade(target, village, groundY);
    drawFrontRoad(target, village, groundY);
}

void StructureManager::drawBackgroundStructures(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    float defaultGroundY = world ? world->getTerrainHeight(viewBounds.left + viewBounds.width * 0.5f) : 500.0f;
    sim::VillageData dummyVillage;
    
    drawRearLawn(target, dummyVillage, defaultGroundY);
    drawRearPalisade(target, dummyVillage, defaultGroundY);

    for (auto& pair : registry.getAllVillages()) {
        const sim::VillageData& v = pair.second;
        if (v.centerX + 3500.f < viewBounds.left || v.centerX - 3500.f > viewBounds.left + viewBounds.width) continue;

        float groundY = world ? world->getTerrainHeight(v.centerX) : 500.0f;
        float yardY = groundY - 130.f;

        float leftHutX = v.centerX - 470.f;
        drawSpriteAnchored(target, rectVillageHut, leftHutX, yardY, 0.72f);

        float leftFireX = v.centerX - 370.f;
        drawSpriteAnchored(target, rectFirePit, leftFireX, yardY, 0.65f);
        drawSpriteAnchored(target, rectFxFire, leftFireX, yardY - 4.f, 0.65f);

        float rightScaffoldX = v.centerX + 480.f;
        drawSpriteAnchored(target, rectLookpostBamboo, rightScaffoldX, yardY, 0.78f);

        float rightFireX = v.centerX + 370.f;
        drawSpriteAnchored(target, rectFirePit, rightFireX, yardY, 0.65f);
        drawSpriteAnchored(target, rectFxFire, rightFireX, yardY - 4.f, 0.65f);
    }
}

void StructureManager::drawMidgroundStructures(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    float defaultGroundY = world ? world->getTerrainHeight(viewBounds.left + viewBounds.width * 0.5f) : 500.0f;
    sim::VillageData primaryVillage;
    for (const auto& pair : registry.getAllVillages()) {
        primaryVillage = pair.second;
        break;
    }

    // Continuous middle palisade and road to infinity
    drawMiddlePalisade(target, primaryVillage, defaultGroundY);
    drawFrontRoad(target, primaryVillage, defaultGroundY);

    for (auto& pair : registry.getAllStructures()) {
        sim::StructureData& s = pair.second;
        if (s.worldX < viewBounds.left - 800.f || s.worldX > viewBounds.left + viewBounds.width + 800.f) continue;

        float groundY = world ? world->getTerrainHeight(s.worldX) : 500.0f;
        sim::VillageData* v = registry.getVillage(s.villageId);
        sim::VillageData fallbackVillage;
        if (!v) v = &fallbackVillage;

        if (s.type == sim::StructureType::EmptyPlot && !s.isUnderConstruction && !s.isFinished) {
            drawEmptyPlot(target, s, groundY);
        } else if (!s.isFinished) {
            if (s.type != sim::StructureType::WatchPlatform && s.type != sim::StructureType::Watchtower &&
                s.type != sim::StructureType::BuilderHut && s.type != sim::StructureType::Nest &&
                s.type != sim::StructureType::StorageHut) {
                drawConstructionSite(target, s, groundY);
            }
        } else {
            switch (s.type) {
                case sim::StructureType::VillageCenter:
                    drawVillageCenter(target, s, *v, groundY);
                    break;
                case sim::StructureType::Throne:
                    drawThrone(target, s, *v, groundY);
                    break;
                case sim::StructureType::ToolRack:
                    drawToolRack(target, s, *v, groundY);
                    break;
                case sim::StructureType::WoodPile:
                case sim::StructureType::StonePile:
                    drawStockpileProps(target, s, *v, groundY);
                    break;
                case sim::StructureType::SimpleBarrier:
                case sim::StructureType::Barricade:
                    drawSimpleBarrier(target, s, *v, groundY);
                    break;
                default:
                    break;
            }
        }
    }
}

void StructureManager::draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    drawBackgroundStructures(target, registry, world, viewBounds);
    drawMidgroundStructures(target, registry, world, viewBounds);
}

void StructureManager::drawForeground(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect&) {
    for (auto& pair : registry.getAllVillages()) {
        sim::VillageData& v = pair.second;
        if (v.isExpandingBorder && v.borderMoverApe != 0) {
            sim::ApeData* mover = registry.getApe(v.borderMoverApe);
            if (mover && mover->isCarryingBorder) {
                float groundY = world ? world->getTerrainHeight(mover->worldX) : 500.0f;

                if (villageTexture && villageTexture->getSize().x > 0) {
                    drawSpriteAnchored(target, rectBorderMonument, mover->worldX, groundY, 0.95f);
                } else {
                    sf::RectangleShape pole(sf::Vector2f(10.f, 90.f));
                    pole.setOrigin(5.f, 45.f);
                    pole.setPosition(mover->worldX, groundY - 45.f);
                    pole.setRotation(v.expandingSideRight ? 35.f : -35.f);
                    pole.setFillColor(sf::Color(85, 55, 28));
                    pole.setOutlineColor(sf::Color(20, 10, 5));
                    pole.setOutlineThickness(2.f);
                    target.draw(pole);

                    sf::CircleShape skullTop(14.f, 6);
                    skullTop.setOrigin(14.f, 14.f);
                    float topOffsetX = v.expandingSideRight ? 26.f : -26.f;
                    skullTop.setPosition(mover->worldX + topOffsetX, groundY - 75.f);
                    skullTop.setFillColor(sf::Color(220, 180, 80));
                    skullTop.setOutlineColor(sf::Color(24, 14, 6));
                    skullTop.setOutlineThickness(2.f);
                    target.draw(skullTop);
                }
            }
        }
    }
}

void StructureManager::drawVillageCenter(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {

    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectCenterBuilding, s.worldX, groundY, 1.0f);
        drawSpriteAnchored(target, rectFxFire, s.worldX - 44.f, groundY - 2.f, 0.88f);
        return;
    }

    sf::Color woodColor(76, 50, 26);
    sf::Color roofColor(148, 108, 50);
    sf::Color bannerColor(195, 42, 32);

    if (village.identity == sim::VillageIdentity::StoneFocused) {
        woodColor = sf::Color(64, 56, 50);
        roofColor = sf::Color(108, 98, 88);
        bannerColor = sf::Color(68, 118, 162);
    } else if (village.identity == sim::VillageIdentity::FoodRich || village.identity == sim::VillageIdentity::Peaceful) {
        roofColor = sf::Color(88, 124, 42);
        bannerColor = sf::Color(222, 168, 44);
    } else if (village.identity == sim::VillageIdentity::Aggressive) {
        bannerColor = sf::Color(212, 26, 18);
    }

    sf::RectangleShape lodgeBase(sf::Vector2f(360.f, 130.f));
    lodgeBase.setOrigin(180.f, 130.f);
    lodgeBase.setPosition(s.worldX, groundY);
    lodgeBase.setFillColor(sf::Color(48, 30, 16));
    lodgeBase.setOutlineColor(sf::Color(16, 10, 5));
    lodgeBase.setOutlineThickness(3.5f);
    target.draw(lodgeBase);

    for (int i = -2; i <= 2; ++i) {
        sf::RectangleShape pillar(sf::Vector2f(28.f, 230.f));
        pillar.setOrigin(14.f, 230.f);
        pillar.setPosition(s.worldX + i * 80.f, groundY);
        pillar.setFillColor(woodColor);
        pillar.setOutlineColor(sf::Color(16, 10, 5));
        pillar.setOutlineThickness(3.f);
        target.draw(pillar);
    }

    sf::RectangleShape crossBeam(sf::Vector2f(410.f, 28.f));
    crossBeam.setOrigin(205.f, 28.f);
    crossBeam.setPosition(s.worldX, groundY - 195.f);
    crossBeam.setFillColor(woodColor);
    crossBeam.setOutlineColor(sf::Color(16, 10, 5));
    crossBeam.setOutlineThickness(3.f);
    target.draw(crossBeam);

    sf::ConvexShape roof(4);
    roof.setPoint(0, sf::Vector2f(-235.f, 0.f));
    roof.setPoint(1, sf::Vector2f(0.f, -115.f));
    roof.setPoint(2, sf::Vector2f(235.f, 0.f));
    roof.setPoint(3, sf::Vector2f(0.f, -48.f));
    roof.setPosition(s.worldX, groundY - 210.f);
    roof.setFillColor(roofColor);
    roof.setOutlineColor(sf::Color(40, 26, 12));
    roof.setOutlineThickness(4.f);
    target.draw(roof);

    sf::RectangleShape hearthRing(sf::Vector2f(110.f, 22.f));
    hearthRing.setOrigin(55.f, 22.f);
    hearthRing.setPosition(s.worldX, groundY);
    hearthRing.setFillColor(sf::Color(68, 62, 56));
    hearthRing.setOutlineColor(sf::Color(22, 22, 22));
    hearthRing.setOutlineThickness(2.5f);
    target.draw(hearthRing);

    sf::ConvexShape hearthFire(3);
    hearthFire.setPoint(0, sf::Vector2f(0.f, -55.f));
    hearthFire.setPoint(1, sf::Vector2f(28.f, 0.f));
    hearthFire.setPoint(2, sf::Vector2f(-28.f, 0.f));
    hearthFire.setPosition(s.worldX, groundY - 12.f);
    hearthFire.setFillColor(sf::Color(245, 115, 22, 240));
    target.draw(hearthFire);

    sf::ConvexShape hearthCore(3);
    hearthCore.setPoint(0, sf::Vector2f(0.f, -32.f));
    hearthCore.setPoint(1, sf::Vector2f(15.f, 0.f));
    hearthCore.setPoint(2, sf::Vector2f(-15.f, 0.f));
    hearthCore.setPosition(s.worldX, groundY - 12.f);
    hearthCore.setFillColor(sf::Color(255, 228, 80, 250));
    target.draw(hearthCore);

    sf::RectangleShape bannerL(sf::Vector2f(48.f, 135.f));
    bannerL.setOrigin(24.f, 0.f);
    bannerL.setPosition(s.worldX - 120.f, groundY - 200.f);
    bannerL.setFillColor(bannerColor);
    bannerL.setOutlineColor(sf::Color(16, 10, 5));
    bannerL.setOutlineThickness(3.f);
    target.draw(bannerL);

    sf::RectangleShape bannerR(sf::Vector2f(48.f, 135.f));
    bannerR.setOrigin(24.f, 0.f);
    bannerR.setPosition(s.worldX + 120.f, groundY - 200.f);
    bannerR.setFillColor(bannerColor);
    bannerR.setOutlineColor(sf::Color(16, 10, 5));
    bannerR.setOutlineThickness(3.f);
    target.draw(bannerR);

    sf::CircleShape skull(20.f, 6);
    skull.setOrigin(20.f, 20.f);
    skull.setPosition(s.worldX, groundY - 170.f);
    skull.setFillColor(sf::Color(238, 232, 218));
    skull.setOutlineColor(sf::Color(22, 22, 22));
    skull.setOutlineThickness(3.f);
    target.draw(skull);
}   

void StructureManager::drawThrone(sf::RenderTarget&, const sim::StructureData&, const sim::VillageData&, float) {
}

void StructureManager::drawToolRack(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {

    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectToolRack, s.worldX, groundY, 1.0f);
        return;
    }

    sf::RectangleShape basePlank(sf::Vector2f(190.f, 16.f));
    basePlank.setOrigin(95.f, 16.f);
    basePlank.setPosition(s.worldX, groundY);
    basePlank.setFillColor(sf::Color(58, 36, 18));
    basePlank.setOutlineColor(sf::Color(20, 12, 6));
    basePlank.setOutlineThickness(2.5f);
    target.draw(basePlank);

    sf::RectangleShape postL(sf::Vector2f(24.f, 140.f));
    postL.setOrigin(12.f, 140.f);
    postL.setPosition(s.worldX - 65.f, groundY);
    postL.setRotation(-6.f);
    postL.setFillColor(sf::Color(80, 52, 28));
    postL.setOutlineColor(sf::Color(20, 12, 6));
    postL.setOutlineThickness(3.f);
    target.draw(postL);

    sf::RectangleShape postR(sf::Vector2f(24.f, 140.f));
    postR.setOrigin(12.f, 140.f);
    postR.setPosition(s.worldX + 65.f, groundY);
    postR.setRotation(6.f);
    postR.setFillColor(sf::Color(80, 52, 28));
    postR.setOutlineColor(sf::Color(20, 12, 6));
    postR.setOutlineThickness(3.f);
    target.draw(postR);

    sf::RectangleShape topBar(sf::Vector2f(180.f, 22.f));
    topBar.setOrigin(90.f, 11.f);
    topBar.setPosition(s.worldX, groundY - 95.f);
    topBar.setFillColor(sf::Color(118, 80, 44));
    topBar.setOutlineColor(sf::Color(24, 15, 8));
    topBar.setOutlineThickness(2.5f);
    target.draw(topBar);

    sf::RectangleShape midBar(sf::Vector2f(165.f, 16.f));
    midBar.setOrigin(82.5f, 8.f);
    midBar.setPosition(s.worldX, groundY - 48.f);
    midBar.setFillColor(sf::Color(98, 64, 34));
    midBar.setOutlineColor(sf::Color(24, 15, 8));
    midBar.setOutlineThickness(2.5f);
    target.draw(midBar);

    for (int i = -2; i <= 2; ++i) {
        sf::RectangleShape peg(sf::Vector2f(10.f, 24.f));
        peg.setOrigin(5.f, 12.f);
        peg.setPosition(s.worldX + i * 36.f, groundY - 95.f);
        peg.setFillColor(sf::Color(48, 30, 16));
        target.draw(peg);
    }

    if (s.axeCount > 0 || s.claimedAxes > 0) {
        sf::RectangleShape handle(sf::Vector2f(90.f, 11.f));
        handle.setOrigin(45.f, 5.5f);
        handle.setPosition(s.worldX, groundY - 98.f);
        handle.setRotation(-18.f);
        handle.setFillColor(sf::Color(148, 102, 54));
        handle.setOutlineColor(sf::Color(40, 24, 12));
        handle.setOutlineThickness(2.5f);
        target.draw(handle);

        sf::ConvexShape blade(4);
        blade.setPoint(0, sf::Vector2f(-20.f, -22.f));
        blade.setPoint(1, sf::Vector2f(18.f, -15.f));
        blade.setPoint(2, sf::Vector2f(22.f, 20.f));
        blade.setPoint(3, sf::Vector2f(-15.f, 18.f));
        blade.setPosition(s.worldX + 30.f, groundY - 110.f);
        blade.setFillColor(sf::Color(122, 128, 134));
        blade.setOutlineColor(sf::Color(32, 34, 38));
        blade.setOutlineThickness(3.f);
        target.draw(blade);

        sf::RectangleShape twine(sf::Vector2f(12.f, 18.f));
        twine.setOrigin(6.f, 9.f);
        twine.setPosition(s.worldX + 18.f, groundY - 105.f);
        twine.setFillColor(sf::Color(212, 178, 122));
        target.draw(twine);
    }
}

void StructureManager::drawStockpileProps(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {

    if (villageTexture && villageTexture->getSize().x > 0) {
        if (s.type == sim::StructureType::WoodPile) {
            drawSpriteAnchored(target, rectMeetingHollowLog, s.worldX, groundY, 1.0f);
        } else {
            drawSpriteAnchored(target, rectMeetingStone, s.worldX, groundY, 1.0f);
        }
        return;
    }

    if (s.type == sim::StructureType::WoodPile) {
        sf::RectangleShape basePlatform(sf::Vector2f(220.f, 16.f));
        basePlatform.setOrigin(110.f, 16.f);
        basePlatform.setPosition(s.worldX, groundY);
        basePlatform.setFillColor(sf::Color(58, 36, 18));
        basePlatform.setOutlineColor(sf::Color(20, 12, 6));
        basePlatform.setOutlineThickness(2.5f);
        target.draw(basePlatform);

        sf::RectangleShape cradleL(sf::Vector2f(20.f, 85.f));
        cradleL.setOrigin(10.f, 85.f);
        cradleL.setPosition(s.worldX - 98.f, groundY);
        cradleL.setRotation(-18.f);
        cradleL.setFillColor(sf::Color(80, 52, 28));
        cradleL.setOutlineColor(sf::Color(20, 12, 6));
        cradleL.setOutlineThickness(3.f);
        target.draw(cradleL);

        sf::RectangleShape cradleR(sf::Vector2f(20.f, 85.f));
        cradleR.setOrigin(10.f, 85.f);
        cradleR.setPosition(s.worldX + 98.f, groundY);
        cradleR.setRotation(18.f);
        cradleR.setFillColor(sf::Color(80, 52, 28));
        cradleR.setOutlineColor(sf::Color(20, 12, 6));
        cradleR.setOutlineThickness(3.f);
        target.draw(cradleR);

        for (int row = 0; row < 3; ++row) {
            int count = 5 - row;
            for (int i = 0; i < count; ++i) {
                sf::CircleShape logEnd(18.f);
                logEnd.setOrigin(18.f, 18.f);
                logEnd.setPosition(s.worldX - 68.f + i * 34.f + row * 17.f, groundY - 18.f - row * 30.f);
                logEnd.setFillColor(sf::Color(168, 128, 78));
                logEnd.setOutlineColor(sf::Color(56, 36, 18));
                logEnd.setOutlineThickness(3.5f);
                target.draw(logEnd);

                sf::CircleShape ring(9.f);
                ring.setOrigin(9.f, 9.f);
                ring.setPosition(logEnd.getPosition());
                ring.setFillColor(sf::Color(138, 98, 54));
                target.draw(ring);
            }
        }
    } else {
        for (int i = -1; i <= 1; ++i) {
            sf::RectangleShape block(sf::Vector2f(50.f, 34.f));
            block.setOrigin(25.f, 34.f);
            block.setPosition(s.worldX + i * 44.f, groundY);
            block.setFillColor(sf::Color(122, 118, 112));
            block.setOutlineColor(sf::Color(40, 38, 36));
            block.setOutlineThickness(3.f);
            target.draw(block);
        }
    }
}

void StructureManager::drawSimpleBarrier(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        sf::Color fenceBgColor(255, 255, 255, 220);
        float scale = 0.95f;
        if (s.worldX < village.centerX) {
            drawSpriteAnchored(target, rectPalisadeLeft, s.worldX, groundY, scale, fenceBgColor);
        } else if (s.worldX > village.centerX) {
            drawSpriteAnchored(target, rectPalisadeRight, s.worldX, groundY, scale, fenceBgColor);
        } else {
            drawSpriteAnchored(target, rectPalisadeMiddle, s.worldX, groundY, scale, fenceBgColor);
        }
        return;
    }

    for (int i = -3; i <= 3; ++i) {
        float stakeH = 135.f + std::abs(i) * 15.f;
        sf::ConvexShape stake(3);
        stake.setPoint(0, sf::Vector2f(0.f, -stakeH));
        stake.setPoint(1, sf::Vector2f(14.f, 0.f));
        stake.setPoint(2, sf::Vector2f(-14.f, 0.f));
        stake.setPosition(s.worldX + i * 30.f, groundY);
        stake.setFillColor(sf::Color(90, 60, 32));
        stake.setOutlineColor(sf::Color(24, 14, 7));
        stake.setOutlineThickness(2.5f);
        target.draw(stake);
    }

    sf::CircleShape sentinel(22.f, 6);
    sentinel.setOrigin(22.f, 22.f);
    sentinel.setPosition(s.worldX, groundY - 155.f);
    sentinel.setFillColor(sf::Color(188, 144, 64));
    sentinel.setOutlineColor(sf::Color(26, 16, 8));
    sentinel.setOutlineThickness(3.f);
    target.draw(sentinel);
}

void StructureManager::drawNest(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectVillageHut, s.worldX, groundY, 0.65f);
        return;
    }

    sf::RectangleShape stilt1(sf::Vector2f(18.f, 75.f));
    stilt1.setOrigin(9.f, 75.f);
    stilt1.setPosition(s.worldX - 52.f, groundY);
    stilt1.setFillColor(sf::Color(78, 52, 28));
    stilt1.setOutlineColor(sf::Color(20, 12, 6));
    stilt1.setOutlineThickness(2.5f);
    target.draw(stilt1);

    sf::RectangleShape stilt2(sf::Vector2f(18.f, 75.f));
    stilt2.setOrigin(9.f, 75.f);
    stilt2.setPosition(s.worldX + 52.f, groundY);
    stilt2.setFillColor(sf::Color(78, 52, 28));
    stilt2.setOutlineColor(sf::Color(20, 12, 6));
    stilt2.setOutlineThickness(2.5f);
    target.draw(stilt2);

    sf::CircleShape outer(80.f);
    outer.setOrigin(80.f, 80.f);
    outer.setPosition(s.worldX, groundY - 58.f);
    outer.setScale(1.4f, 0.5f);
    outer.setFillColor(sf::Color(165, 125, 48));
    outer.setOutlineColor(sf::Color(68, 45, 18));
    outer.setOutlineThickness(4.f);
    target.draw(outer);
}

void StructureManager::drawStorageHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectVillageHut, s.worldX, groundY, 0.70f);
        return;
    }

    sf::RectangleShape base(sf::Vector2f(220.f, 110.f));
    base.setOrigin(110.f, 110.f);
    base.setPosition(s.worldX, groundY);
    base.setFillColor(sf::Color(88, 58, 30));
    base.setOutlineColor(sf::Color(30, 18, 9));
    base.setOutlineThickness(3.f);
    target.draw(base);

    sf::RectangleShape door(sf::Vector2f(52.f, 75.f));
    door.setOrigin(26.f, 75.f);
    door.setPosition(s.worldX, groundY);
    door.setFillColor(sf::Color(40, 24, 12));
    door.setOutlineColor(sf::Color(20, 12, 6));
    door.setOutlineThickness(2.5f);
    target.draw(door);

    sf::ConvexShape roof(3);
    roof.setPoint(0, sf::Vector2f(0.f, -75.f));
    roof.setPoint(1, sf::Vector2f(-135.f, 0.f));
    roof.setPoint(2, sf::Vector2f(135.f, 0.f));
    roof.setPosition(s.worldX, groundY - 110.f);
    roof.setFillColor(sf::Color(142, 108, 52));
    roof.setOutlineColor(sf::Color(48, 32, 16));
    roof.setOutlineThickness(3.5f);
    target.draw(roof);
}

void StructureManager::drawWatchPlatform(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectLookpostBamboo, s.worldX, groundY, 0.75f);
        return;
    }

    sf::RectangleShape p1(sf::Vector2f(22.f, 270.f));
    p1.setOrigin(11.f, 270.f);
    p1.setPosition(s.worldX - 52.f, groundY);
    p1.setFillColor(sf::Color(76, 52, 28));
    p1.setOutlineColor(sf::Color::Black);
    p1.setOutlineThickness(2.5f);
    target.draw(p1);

    sf::RectangleShape p2(sf::Vector2f(22.f, 270.f));
    p2.setOrigin(11.f, 270.f);
    p2.setPosition(s.worldX + 52.f, groundY);
    p2.setFillColor(sf::Color(76, 52, 28));
    p2.setOutlineColor(sf::Color::Black);
    p2.setOutlineThickness(2.5f);
    target.draw(p2);

    for (float y = groundY - 40.f; y > groundY - 240.f; y -= 40.f) {
        sf::RectangleShape rung(sf::Vector2f(105.f, 10.f));
        rung.setOrigin(52.5f, 5.f);
        rung.setPosition(s.worldX, y);
        rung.setFillColor(sf::Color(122, 88, 48));
        rung.setOutlineColor(sf::Color(26, 16, 8));
        rung.setOutlineThickness(2.f);
        target.draw(rung);
    }

    sf::RectangleShape platform(sf::Vector2f(160.f, 22.f));
    platform.setOrigin(80.f, 22.f);
    platform.setPosition(s.worldX, groundY - 250.f);
    platform.setFillColor(sf::Color(112, 78, 42));
    platform.setOutlineColor(sf::Color::Black);
    platform.setOutlineThickness(3.f);
    target.draw(platform);
}

void StructureManager::drawBuilderHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectLookpostBamboo, s.worldX, groundY, 0.75f);
        return;
    }

    sf::RectangleShape postL(sf::Vector2f(22.f, 125.f));
    postL.setOrigin(11.f, 125.f);
    postL.setPosition(s.worldX - 80.f, groundY);
    postL.setFillColor(sf::Color(82, 54, 30));
    postL.setOutlineColor(sf::Color(26, 16, 8));
    postL.setOutlineThickness(2.5f);
    target.draw(postL);

    sf::RectangleShape postR(sf::Vector2f(22.f, 125.f));
    postR.setOrigin(11.f, 125.f);
    postR.setPosition(s.worldX + 80.f, groundY);
    postR.setFillColor(sf::Color(82, 54, 30));
    postR.setOutlineColor(sf::Color(26, 16, 8));
    postR.setOutlineThickness(2.5f);
    target.draw(postR);

    sf::RectangleShape roof(sf::Vector2f(205.f, 26.f));
    roof.setOrigin(102.5f, 26.f);
    roof.setPosition(s.worldX, groundY - 120.f);
    roof.setRotation(-4.f);
    roof.setFillColor(sf::Color(128, 92, 50));
    roof.setOutlineColor(sf::Color(42, 26, 14));
    roof.setOutlineThickness(3.f);
    target.draw(roof);
}

void StructureManager::drawBonfire(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectFirePit, s.worldX, groundY, 0.72f);
        drawSpriteAnchored(target, rectFxFire, s.worldX, groundY - 4.f, 0.72f);
        return;
    }

    sf::CircleShape stoneRing(55.f, 8);
    stoneRing.setOrigin(55.f, 55.f);
    stoneRing.setPosition(s.worldX, groundY - 5.f);
    stoneRing.setScale(1.5f, 0.5f);
    stoneRing.setFillColor(sf::Color(78, 74, 68));
    stoneRing.setOutlineColor(sf::Color(26, 26, 26));
    stoneRing.setOutlineThickness(3.5f);
    target.draw(stoneRing);

    sf::ConvexShape fireOuter(3);
    fireOuter.setPoint(0, sf::Vector2f(0.f, -70.f));
    fireOuter.setPoint(1, sf::Vector2f(30.f, 0.f));
    fireOuter.setPoint(2, sf::Vector2f(-30.f, 0.f));
    fireOuter.setPosition(s.worldX, groundY - 14.f);
    fireOuter.setFillColor(sf::Color(238, 92, 22, 235));
    target.draw(fireOuter);
}

void StructureManager::drawConstructionSite(sf::RenderTarget& target, const sim::StructureData& s, float groundY) {
    float progressRatio = std::clamp(s.progress / std::max(1.f, s.maxProgress), 0.f, 1.f);

    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectLookpostBamboo, s.worldX, groundY, 0.75f, sf::Color(255, 255, 255, static_cast<sf::Uint8>(120 + progressRatio * 135)));
    } else {
        sf::RectangleShape stakeL(sf::Vector2f(14.f, 65.f));
        stakeL.setOrigin(7.f, 65.f);
        stakeL.setPosition(s.worldX - 100.f, groundY);
        stakeL.setFillColor(sf::Color(90, 60, 30));
        stakeL.setOutlineColor(sf::Color(26, 16, 8));
        stakeL.setOutlineThickness(2.5f);
        target.draw(stakeL);

        sf::RectangleShape stakeR(sf::Vector2f(14.f, 65.f));
        stakeR.setOrigin(7.f, 65.f);
        stakeR.setPosition(s.worldX + 100.f, groundY);
        stakeR.setFillColor(sf::Color(90, 60, 30));
        stakeR.setOutlineColor(sf::Color(26, 16, 8));
        stakeR.setOutlineThickness(2.5f);
        target.draw(stakeR);

        sf::RectangleShape cord(sf::Vector2f(200.f, 5.f));
        cord.setOrigin(100.f, 2.5f);
        cord.setPosition(s.worldX, groundY - 42.f);
        cord.setFillColor(sf::Color(224, 194, 104));
        target.draw(cord);

        if (progressRatio >= 0.25f) {
            sf::RectangleShape foundation(sf::Vector2f(175.f, 24.f));
            foundation.setOrigin(87.5f, 24.f);
            foundation.setPosition(s.worldX, groundY - 5.f);
            foundation.setFillColor(sf::Color(100, 70, 38));
            foundation.setOutlineColor(sf::Color(36, 22, 11));
            foundation.setOutlineThickness(2.5f);
            target.draw(foundation);
        }

        if (progressRatio >= 0.50f) {
            sf::RectangleShape frameL(sf::Vector2f(16.f, 100.f));
            frameL.setOrigin(8.f, 100.f);
            frameL.setPosition(s.worldX - 65.f, groundY - 24.f);
            frameL.setFillColor(sf::Color(122, 88, 50));
            target.draw(frameL);

            sf::RectangleShape frameR(sf::Vector2f(16.f, 100.f));
            frameR.setOrigin(8.f, 100.f);
            frameR.setPosition(s.worldX + 65.f, groundY - 24.f);
            frameR.setFillColor(sf::Color(122, 88, 50));
            target.draw(frameR);
        }
    }

    if (s.isUnderConstruction) {
        float barW = 110.f;
        sf::RectangleShape barBg(sf::Vector2f(barW, 12.f));
        barBg.setOrigin(barW * 0.5f, 6.f);
        barBg.setPosition(s.worldX, groundY - 130.f);
        barBg.setFillColor(sf::Color(32, 22, 14, 235));
        barBg.setOutlineColor(sf::Color(168, 134, 68));
        barBg.setOutlineThickness(2.5f);
        target.draw(barBg);

        sf::RectangleShape barFill(sf::Vector2f(barW * progressRatio, 12.f));
        barFill.setOrigin(barW * 0.5f, 6.f);
        barFill.setPosition(s.worldX, groundY - 130.f);
        barFill.setFillColor(sf::Color(228, 184, 52));
        target.draw(barFill);
    }
}

void StructureManager::setGroundTexture(const sf::Texture& tex) {
    groundTexture = &tex;
}

void StructureManager::setRearLawnTexture(const sf::Texture& tex) {
    rearLawnTexture = &tex;
}

void StructureManager::setShadowParams(float shearX, float projY, sf::Color color) {
    shadowShearX = shearX;
    shadowProjY = projY;
    shadowColor = color;
}