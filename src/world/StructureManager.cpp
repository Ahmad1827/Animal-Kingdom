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
    sprite.setColor(color);
    target.draw(sprite);
}

static void drawEmptyPlot(sf::RenderTarget& target, const sim::StructureData& s, float groundY) {
    sf::RectangleShape plotBase(sf::Vector2f(90.f, 4.f));
    plotBase.setOrigin(45.f, 2.f);
    plotBase.setPosition(s.worldX, groundY - 1.f);
    plotBase.setFillColor(sf::Color(80, 55, 30, 160));
    target.draw(plotBase);

    for (int i = -1; i <= 1; i += 2) {
        sf::RectangleShape peg(sf::Vector2f(4.f, 20.f));
        peg.setOrigin(2.f, 20.f);
        peg.setPosition(s.worldX + i * 35.f, groundY);
        peg.setFillColor(sf::Color(85, 55, 28));
        peg.setOutlineColor(sf::Color(20, 10, 5));
        peg.setOutlineThickness(1.f);
        target.draw(peg);
    }
}

void StructureManager::drawMeetingGround(sf::RenderTarget& target, float worldX, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectMeetingRootLog, worldX - 110.f, groundY, 0.35f);
        drawSpriteAnchored(target, rectMeetingStone, worldX, groundY, 0.35f);
        drawSpriteAnchored(target, rectMeetingHollowLog, worldX + 110.f, groundY, 0.35f);
    }
}

void StructureManager::draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    for (auto& pair : registry.getAllVillages()) {
        const sim::VillageData& v = pair.second;
        if (v.centerX + 1400.f < viewBounds.left || v.centerX - 1400.f > viewBounds.left + viewBounds.width) continue;
        float groundY = world ? world->getTerrainHeight(v.centerX) : 500.0f;
        drawSettlementFootprint(target, v, groundY);
    }

    for (auto& pair : registry.getAllStructures()) {
        sim::StructureData& s = pair.second;
        if (s.type == sim::StructureType::SimpleBarrier) continue;
        if (s.worldX < viewBounds.left - 500.f || s.worldX > viewBounds.left + viewBounds.width + 500.f) continue;

        float groundY = world ? world->getTerrainHeight(s.worldX) : 500.0f;
        sim::VillageData* v = registry.getVillage(s.villageId);
        sim::VillageData fallbackVillage;
        if (!v) v = &fallbackVillage;

        if (s.type == sim::StructureType::EmptyPlot && !s.isUnderConstruction && !s.isFinished) {
            drawEmptyPlot(target, s, groundY);
        } else if (!s.isFinished) {
            drawConstructionSite(target, s, groundY);
        } else {
            switch (s.type) {
                case sim::StructureType::VillageCenter:
                    drawVillageCenter(target, s, *v, groundY);
                    break;
                case sim::StructureType::Nest:
                    drawNest(target, s, *v, groundY);
                    break;
                case sim::StructureType::StorageHut:
                    drawStorageHut(target, s, *v, groundY);
                    break;
                case sim::StructureType::WatchPlatform:
                    drawWatchPlatform(target, s, *v, groundY);
                    break;
                case sim::StructureType::BuilderHut:
                    drawBuilderHut(target, s, *v, groundY);
                    break;
                case sim::StructureType::Bonfire:
                    drawBonfire(target, s, *v, groundY);
                    break;
                case sim::StructureType::ToolRack:
                    drawToolRack(target, s, *v, groundY);
                    break;
                case sim::StructureType::WoodPile:
                case sim::StructureType::StonePile:
                    drawStockpileProps(target, s, *v, groundY);
                    break;
                default:
                    break;
            }
        }
    }
}

void StructureManager::drawForeground(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    for (auto& pair : registry.getAllVillages()) {
        sim::VillageData& v = pair.second;
        if (v.isExpandingBorder && v.borderMoverApe != 0) {
            sim::ApeData* mover = registry.getApe(v.borderMoverApe);
            if (mover && mover->isCarryingBorder) {
                float groundY = world ? world->getTerrainHeight(mover->worldX) : 500.0f;

                if (villageTexture && villageTexture->getSize().x > 0) {
                    drawSpriteAnchored(target, rectBorderMonument, mover->worldX, groundY, 0.35f);
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

    for (auto& pair : registry.getAllStructures()) {
        sim::StructureData& s = pair.second;
        if (s.type != sim::StructureType::SimpleBarrier) continue;
        if (s.worldX < viewBounds.left - 500.f || s.worldX > viewBounds.left + viewBounds.width + 500.f) continue;

        float groundY = world ? world->getTerrainHeight(s.worldX) : 500.0f;
        sim::VillageData* v = registry.getVillage(s.villageId);
        sim::VillageData fallbackVillage;
        if (!v) v = &fallbackVillage;

        if (!s.isFinished) {
            drawConstructionSite(target, s, groundY);
        } else {
            drawSimpleBarrier(target, s, *v, groundY);
        }
    }
}

void StructureManager::drawSettlementFootprint(sf::RenderTarget& target, const sim::VillageData& village, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) return;

    float footprintW = 2400.f;
    float startX = village.centerX - footprintW / 2.f;

    sf::RectangleShape dirtBed(sf::Vector2f(footprintW, 36.f));
    dirtBed.setPosition(startX, groundY - 4.f);
    dirtBed.setFillColor(sf::Color(32, 22, 14, 235));
    target.draw(dirtBed);

    sf::RectangleShape innerMat(sf::Vector2f(1100.f, 12.f));
    innerMat.setOrigin(550.f, 0.f);
    innerMat.setPosition(village.centerX, groundY - 3.f);
    innerMat.setFillColor(sf::Color(118, 88, 48, 220));
    target.draw(innerMat);
}

void StructureManager::drawVillageCenter(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectCenterBuilding, s.worldX, groundY, 0.35f);
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

void StructureManager::drawToolRack(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectToolRack, s.worldX, groundY, 0.35f);
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
}

void StructureManager::drawStockpileProps(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectVillageHut, s.worldX, groundY, 0.35f);
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
        if (s.worldX < village.centerX) {
            drawSpriteAnchored(target, rectPalisadeLeft, s.worldX, groundY, 0.35f);
        } else if (s.worldX > village.centerX) {
            drawSpriteAnchored(target, rectPalisadeRight, s.worldX, groundY, 0.35f);
        } else {
            drawSpriteAnchored(target, rectPalisadeMiddle, s.worldX, groundY, 0.35f);
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
}

void StructureManager::drawNest(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectVillageHut, s.worldX, groundY, 0.35f);
        return;
    }

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
        drawSpriteAnchored(target, rectVillageHut, s.worldX, groundY, 0.35f);
        return;
    }

    sf::RectangleShape base(sf::Vector2f(220.f, 110.f));
    base.setOrigin(110.f, 110.f);
    base.setPosition(s.worldX, groundY);
    base.setFillColor(sf::Color(88, 58, 30));
    base.setOutlineColor(sf::Color(30, 18, 9));
    base.setOutlineThickness(3.f);
    target.draw(base);
}

void StructureManager::drawWatchPlatform(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectLookpostBamboo, s.worldX, groundY, 0.35f);
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
}

void StructureManager::drawBuilderHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectVillageHut, s.worldX, groundY, 0.35f);
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
}

void StructureManager::drawBonfire(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData&, float groundY) {
    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectFirePit, s.worldX, groundY, 0.35f);
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
}

void StructureManager::drawConstructionSite(sf::RenderTarget& target, const sim::StructureData& s, float groundY) {
    float progressRatio = std::clamp(s.progress / std::max(1.f, s.maxProgress), 0.f, 1.f);

    if (villageTexture && villageTexture->getSize().x > 0) {
        drawSpriteAnchored(target, rectVillageHut, s.worldX, groundY, 0.35f, sf::Color(255, 255, 255, static_cast<sf::Uint8>(60 + progressRatio * 140)));
    }

    if (s.isUnderConstruction) {
        float barW = 80.f;
        sf::RectangleShape barBg(sf::Vector2f(barW, 8.f));
        barBg.setOrigin(barW * 0.5f, 4.f);
        barBg.setPosition(s.worldX, groundY - 100.f);
        barBg.setFillColor(sf::Color(32, 22, 14, 235));
        barBg.setOutlineColor(sf::Color(168, 134, 68));
        barBg.setOutlineThickness(1.5f);
        target.draw(barBg);

        sf::RectangleShape barFill(sf::Vector2f(barW * progressRatio, 8.f));
        barFill.setOrigin(barW * 0.5f, 4.f);
        barFill.setPosition(s.worldX, groundY - 100.f);
        barFill.setFillColor(sf::Color(228, 184, 52));
        target.draw(barFill);
    }
}