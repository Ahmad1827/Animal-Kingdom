#include "world/StructureManager.h"
#include <cmath>
#include <algorithm>

StructureManager::StructureManager() {}

void StructureManager::update(float dt, sim::SimulationRegistry& registry) {}

void StructureManager::draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    for (auto& pair : registry.getAllVillages()) {
        const sim::VillageData& v = pair.second;
        if (v.centerX + 600.f < viewBounds.left || v.centerX - 600.f > viewBounds.left + viewBounds.width) continue;
        drawSettlementFootprint(target, v, 500.0f);
    }

    for (auto& pair : registry.getAllStructures()) {
        sim::StructureData& s = pair.second;
        if (s.worldX < viewBounds.left - 300.f || s.worldX > viewBounds.left + viewBounds.width + 300.f) continue;

        float groundY = 500.0f;
        sim::VillageData* v = registry.getVillage(s.villageId);
        sim::VillageData fallbackVillage;
        if (!v) v = &fallbackVillage;

        if (!s.isFinished) {
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
                case sim::StructureType::SimpleBarrier:
                    drawSimpleBarrier(target, s, *v, groundY);
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

void StructureManager::drawSettlementFootprint(sf::RenderTarget& target, const sim::VillageData& village, float groundY) {
    float footprintW = 860.f;
    float startX = village.centerX - footprintW / 2.f;

    sf::RectangleShape dirtBed(sf::Vector2f(footprintW, 16.f));
    dirtBed.setPosition(startX, groundY - 2.f);
    dirtBed.setFillColor(sf::Color(44, 30, 18, 220));
    target.draw(dirtBed);

    sf::RectangleShape innerMat(sf::Vector2f(340.f, 6.f));
    innerMat.setOrigin(170.f, 0.f);
    innerMat.setPosition(village.centerX, groundY - 1.f);
    innerMat.setFillColor(sf::Color(140, 108, 58, 195));
    target.draw(innerMat);

    for (int i = -4; i <= 4; ++i) {
        if (i == 0) continue;
        sf::CircleShape stone(4.f + (std::abs(i) % 3) * 2.f, 5);
        stone.setOrigin(stone.getRadius(), stone.getRadius());
        stone.setPosition(village.centerX + i * 95.f, groundY - 2.f);
        stone.setFillColor(sf::Color(85, 80, 72));
        target.draw(stone);
    }
}

void StructureManager::drawVillageCenter(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    sf::Color woodColor(80, 52, 28);
    sf::Color roofColor(135, 100, 50);
    sf::Color bannerColor(185, 45, 35);

    if (village.identity == sim::VillageIdentity::StoneFocused) {
        woodColor = sf::Color(65, 58, 52);
        roofColor = sf::Color(105, 95, 85);
        bannerColor = sf::Color(70, 120, 160);
    } else if (village.identity == sim::VillageIdentity::FoodRich || village.identity == sim::VillageIdentity::Peaceful) {
        roofColor = sf::Color(85, 120, 42);
        bannerColor = sf::Color(215, 165, 45);
    } else if (village.identity == sim::VillageIdentity::Aggressive) {
        bannerColor = sf::Color(195, 25, 18);
    }

    sf::RectangleShape leftPillar(sf::Vector2f(16.f, 125.f));
    leftPillar.setOrigin(8.f, 125.f);
    leftPillar.setPosition(s.worldX - 70.f, groundY);
    leftPillar.setFillColor(woodColor);
    leftPillar.setOutlineColor(sf::Color::Black);
    leftPillar.setOutlineThickness(1.5f);
    target.draw(leftPillar);

    sf::RectangleShape rightPillar(sf::Vector2f(16.f, 125.f));
    rightPillar.setOrigin(8.f, 125.f);
    rightPillar.setPosition(s.worldX + 70.f, groundY);
    rightPillar.setFillColor(woodColor);
    rightPillar.setOutlineColor(sf::Color::Black);
    rightPillar.setOutlineThickness(1.5f);
    target.draw(rightPillar);

    sf::RectangleShape centerBeam(sf::Vector2f(190.f, 14.f));
    centerBeam.setOrigin(95.f, 14.f);
    centerBeam.setPosition(s.worldX, groundY - 100.f);
    centerBeam.setFillColor(woodColor);
    centerBeam.setOutlineColor(sf::Color::Black);
    centerBeam.setOutlineThickness(1.5f);
    target.draw(centerBeam);

    sf::ConvexShape roof(4);
    roof.setPoint(0, sf::Vector2f(-110.f, 0.f));
    roof.setPoint(1, sf::Vector2f(0.f, -50.f));
    roof.setPoint(2, sf::Vector2f(110.f, 0.f));
    roof.setPoint(3, sf::Vector2f(0.f, -22.f));
    roof.setPosition(s.worldX, groundY - 110.f);
    roof.setFillColor(roofColor);
    roof.setOutlineColor(sf::Color(40, 25, 12));
    roof.setOutlineThickness(2.f);
    target.draw(roof);

    sf::RectangleShape throne(sf::Vector2f(56.f, 12.f));
    throne.setOrigin(28.f, 12.f);
    throne.setPosition(s.worldX, groundY - 30.f);
    throne.setFillColor(sf::Color(100, 70, 40));
    throne.setOutlineColor(sf::Color::Black);
    throne.setOutlineThickness(1.f);
    target.draw(throne);

    sf::RectangleShape throneBack(sf::Vector2f(12.f, 38.f));
    throneBack.setOrigin(6.f, 38.f);
    throneBack.setPosition(s.worldX - 20.f, groundY - 30.f);
    throneBack.setFillColor(sf::Color(72, 48, 28));
    target.draw(throneBack);

    sf::RectangleShape banner(sf::Vector2f(28.f, 65.f));
    banner.setOrigin(14.f, 0.f);
    banner.setPosition(s.worldX, groundY - 105.f);
    banner.setFillColor(bannerColor);
    banner.setOutlineColor(sf::Color(25, 18, 10));
    banner.setOutlineThickness(1.5f);
    target.draw(banner);

    sf::CircleShape skull(8.f, 6);
    skull.setOrigin(8.f, 8.f);
    skull.setPosition(s.worldX, groundY - 92.f);
    skull.setFillColor(sf::Color(225, 220, 205));
    skull.setOutlineColor(sf::Color::Black);
    skull.setOutlineThickness(1.f);
    target.draw(skull);
}

void StructureManager::drawNest(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    sf::RectangleShape stilt1(sf::Vector2f(8.f, 32.f));
    stilt1.setOrigin(4.f, 32.f);
    stilt1.setPosition(s.worldX - 26.f, groundY);
    stilt1.setFillColor(sf::Color(78, 52, 28));
    target.draw(stilt1);

    sf::RectangleShape stilt2(sf::Vector2f(8.f, 32.f));
    stilt2.setOrigin(4.f, 32.f);
    stilt2.setPosition(s.worldX + 26.f, groundY);
    stilt2.setFillColor(sf::Color(78, 52, 28));
    target.draw(stilt2);

    sf::CircleShape outer(34.f);
    outer.setOrigin(34.f, 34.f);
    outer.setPosition(s.worldX, groundY - 22.f);
    outer.setScale(1.35f, 0.45f);
    outer.setFillColor(sf::Color(165, 125, 48));
    outer.setOutlineColor(sf::Color(68, 45, 18));
    outer.setOutlineThickness(2.f);
    target.draw(outer);

    sf::CircleShape inner(22.f);
    inner.setOrigin(22.f, 22.f);
    inner.setPosition(s.worldX, groundY - 24.f);
    inner.setScale(1.25f, 0.4f);
    inner.setFillColor(sf::Color(125, 90, 32));
    target.draw(inner);

    sf::CircleShape leaves(12.f);
    leaves.setOrigin(12.f, 12.f);
    leaves.setPosition(s.worldX, groundY - 25.f);
    leaves.setScale(1.2f, 0.35f);
    leaves.setFillColor(sf::Color(70, 110, 42));
    target.draw(leaves);
}

void StructureManager::drawStorageHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    sf::RectangleShape base(sf::Vector2f(105.f, 52.f));
    base.setOrigin(52.5f, 52.f);
    base.setPosition(s.worldX, groundY);
    base.setFillColor(sf::Color(88, 58, 30));
    base.setOutlineColor(sf::Color(32, 18, 8));
    base.setOutlineThickness(1.5f);
    target.draw(base);

    sf::RectangleShape door(sf::Vector2f(26.f, 38.f));
    door.setOrigin(13.f, 38.f);
    door.setPosition(s.worldX, groundY);
    door.setFillColor(sf::Color(42, 26, 12));
    target.draw(door);

    sf::ConvexShape roof(3);
    roof.setPoint(0, sf::Vector2f(0.f, -36.f));
    roof.setPoint(1, sf::Vector2f(-66.f, 0.f));
    roof.setPoint(2, sf::Vector2f(66.f, 0.f));
    roof.setPosition(s.worldX, groundY - 52.f);
    roof.setFillColor(sf::Color(135, 100, 48));
    roof.setOutlineColor(sf::Color(48, 32, 16));
    roof.setOutlineThickness(2.f);
    target.draw(roof);

    for (int i = -1; i <= 1; i += 2) {
        sf::CircleShape basket(7.f);
        basket.setOrigin(7.f, 7.f);
        basket.setPosition(s.worldX + i * 38.f, groundY - 10.f);
        basket.setFillColor(sf::Color(175, 125, 42));
        target.draw(basket);

        sf::CircleShape fruit(3.5f);
        fruit.setOrigin(3.5f, 3.5f);
        fruit.setPosition(s.worldX + i * 38.f, groundY - 13.f);
        fruit.setFillColor(sf::Color(215, 75, 35));
        target.draw(fruit);
    }
}

void StructureManager::drawWatchPlatform(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    sf::RectangleShape p1(sf::Vector2f(10.f, 150.f));
    p1.setOrigin(5.f, 150.f);
    p1.setPosition(s.worldX - 24.f, groundY);
    p1.setFillColor(sf::Color(72, 48, 26));
    p1.setOutlineColor(sf::Color::Black);
    p1.setOutlineThickness(1.f);
    target.draw(p1);

    sf::RectangleShape p2(sf::Vector2f(10.f, 150.f));
    p2.setOrigin(5.f, 150.f);
    p2.setPosition(s.worldX + 24.f, groundY);
    p2.setFillColor(sf::Color(72, 48, 26));
    p2.setOutlineColor(sf::Color::Black);
    p2.setOutlineThickness(1.f);
    target.draw(p2);

    for (float y = groundY - 25.f; y > groundY - 130.f; y -= 22.f) {
        sf::RectangleShape rung(sf::Vector2f(48.f, 4.f));
        rung.setOrigin(24.f, 2.f);
        rung.setPosition(s.worldX, y);
        rung.setFillColor(sf::Color(115, 80, 42));
        target.draw(rung);
    }

    sf::RectangleShape platform(sf::Vector2f(76.f, 10.f));
    platform.setOrigin(38.f, 10.f);
    platform.setPosition(s.worldX, groundY - 135.f);
    platform.setFillColor(sf::Color(105, 72, 38));
    platform.setOutlineColor(sf::Color::Black);
    platform.setOutlineThickness(1.5f);
    target.draw(platform);

    sf::RectangleShape rail(sf::Vector2f(76.f, 4.f));
    rail.setOrigin(38.f, 4.f);
    rail.setPosition(s.worldX, groundY - 155.f);
    rail.setFillColor(sf::Color(135, 95, 50));
    target.draw(rail);
}

void StructureManager::drawBuilderHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    sf::RectangleShape postL(sf::Vector2f(10.f, 62.f));
    postL.setOrigin(5.f, 62.f);
    postL.setPosition(s.worldX - 42.f, groundY);
    postL.setFillColor(sf::Color(78, 50, 26));
    target.draw(postL);

    sf::RectangleShape postR(sf::Vector2f(10.f, 62.f));
    postR.setOrigin(5.f, 62.f);
    postR.setPosition(s.worldX + 42.f, groundY);
    postR.setFillColor(sf::Color(78, 50, 26));
    target.draw(postR);

    sf::RectangleShape roof(sf::Vector2f(105.f, 12.f));
    roof.setOrigin(52.5f, 12.f);
    roof.setPosition(s.worldX, groundY - 58.f);
    roof.setRotation(-4.f);
    roof.setFillColor(sf::Color(120, 85, 45));
    roof.setOutlineColor(sf::Color(38, 22, 10));
    roof.setOutlineThickness(1.5f);
    target.draw(roof);

    sf::RectangleShape bench(sf::Vector2f(48.f, 16.f));
    bench.setOrigin(24.f, 16.f);
    bench.setPosition(s.worldX, groundY);
    bench.setFillColor(sf::Color(100, 70, 38));
    target.draw(bench);

    sf::CircleShape anvil(7.f, 6);
    anvil.setOrigin(7.f, 7.f);
    anvil.setPosition(s.worldX + 10.f, groundY - 18.f);
    anvil.setFillColor(sf::Color(95, 95, 95));
    target.draw(anvil);
}

void StructureManager::drawBonfire(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    sf::CircleShape stoneRing(24.f, 8);
    stoneRing.setOrigin(24.f, 24.f);
    stoneRing.setPosition(s.worldX, groundY - 2.f);
    stoneRing.setScale(1.4f, 0.45f);
    stoneRing.setFillColor(sf::Color(72, 68, 62));
    stoneRing.setOutlineColor(sf::Color(28, 28, 28));
    stoneRing.setOutlineThickness(2.f);
    target.draw(stoneRing);

    sf::RectangleShape log1(sf::Vector2f(32.f, 7.f));
    log1.setOrigin(16.f, 3.5f);
    log1.setPosition(s.worldX, groundY - 5.f);
    log1.setRotation(18.f);
    log1.setFillColor(sf::Color(52, 30, 14));
    target.draw(log1);

    sf::RectangleShape log2(sf::Vector2f(32.f, 7.f));
    log2.setOrigin(16.f, 3.5f);
    log2.setPosition(s.worldX, groundY - 5.f);
    log2.setRotation(-22.f);
    log2.setFillColor(sf::Color(42, 24, 10));
    target.draw(log2);

    sf::ConvexShape fireOuter(3);
    fireOuter.setPoint(0, sf::Vector2f(0.f, -30.f));
    fireOuter.setPoint(1, sf::Vector2f(13.f, 0.f));
    fireOuter.setPoint(2, sf::Vector2f(-13.f, 0.f));
    fireOuter.setPosition(s.worldX, groundY - 7.f);
    fireOuter.setFillColor(sf::Color(230, 85, 18, 220));
    target.draw(fireOuter);

    sf::ConvexShape fireInner(3);
    fireInner.setPoint(0, sf::Vector2f(0.f, -16.f));
    fireInner.setPoint(1, sf::Vector2f(7.f, 0.f));
    fireInner.setPoint(2, sf::Vector2f(-7.f, 0.f));
    fireInner.setPosition(s.worldX, groundY - 7.f);
    fireInner.setFillColor(sf::Color(255, 210, 55, 240));
    target.draw(fireInner);
}

void StructureManager::drawSimpleBarrier(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    for (int i = -2; i <= 2; ++i) {
        float stakeH = 48.f + std::abs(i) * 5.f;
        sf::ConvexShape stake(3);
        stake.setPoint(0, sf::Vector2f(0.f, -stakeH));
        stake.setPoint(1, sf::Vector2f(5.5f, 0.f));
        stake.setPoint(2, sf::Vector2f(-5.5f, 0.f));
        stake.setPosition(s.worldX + i * 13.f, groundY);
        stake.setFillColor(sf::Color(92, 62, 34));
        stake.setOutlineColor(sf::Color(32, 18, 8));
        stake.setOutlineThickness(1.f);
        target.draw(stake);
    }
}

void StructureManager::drawStockpileProps(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    if (s.type == sim::StructureType::WoodPile) {
        for (int row = 0; row < 3; ++row) {
            int count = 4 - row;
            for (int i = 0; i < count; ++i) {
                sf::CircleShape logEnd(5.5f);
                logEnd.setOrigin(5.5f, 5.5f);
                logEnd.setPosition(s.worldX - 16.f + i * 11.f + row * 5.5f, groundY - 5.5f - row * 9.f);
                logEnd.setFillColor(sf::Color(160, 120, 70));
                logEnd.setOutlineColor(sf::Color(60, 38, 18));
                logEnd.setOutlineThickness(1.5f);
                target.draw(logEnd);
            }
        }
    } else {
        for (int i = -1; i <= 1; ++i) {
            sf::RectangleShape block(sf::Vector2f(16.f, 12.f));
            block.setOrigin(8.f, 12.f);
            block.setPosition(s.worldX + i * 14.f, groundY);
            block.setFillColor(sf::Color(115, 110, 105));
            block.setOutlineColor(sf::Color(42, 40, 38));
            block.setOutlineThickness(1.5f);
            target.draw(block);
        }
    }
}

void StructureManager::drawConstructionSite(sf::RenderTarget& target, const sim::StructureData& s, float groundY) {
    float progressRatio = std::clamp(s.progress / std::max(1.f, s.maxProgress), 0.f, 1.f);

    sf::RectangleShape stakeL(sf::Vector2f(6.f, 26.f));
    stakeL.setOrigin(3.f, 26.f);
    stakeL.setPosition(s.worldX - 35.f, groundY);
    stakeL.setFillColor(sf::Color(88, 58, 28));
    target.draw(stakeL);

    sf::RectangleShape stakeR(sf::Vector2f(6.f, 26.f));
    stakeR.setOrigin(3.f, 26.f);
    stakeR.setPosition(s.worldX + 35.f, groundY);
    stakeR.setFillColor(sf::Color(88, 58, 28));
    target.draw(stakeR);

    sf::RectangleShape cord(sf::Vector2f(70.f, 2.f));
    cord.setOrigin(35.f, 1.f);
    cord.setPosition(s.worldX, groundY - 18.f);
    cord.setFillColor(sf::Color(205, 175, 85));
    target.draw(cord);

    if (progressRatio >= 0.33f) {
        sf::RectangleShape scaf(sf::Vector2f(55.f, 32.f));
        scaf.setOrigin(27.5f, 32.f);
        scaf.setPosition(s.worldX, groundY);
        scaf.setFillColor(sf::Color::Transparent);
        scaf.setOutlineColor(sf::Color(140, 90, 42));
        scaf.setOutlineThickness(2.f);
        target.draw(scaf);
    }
}