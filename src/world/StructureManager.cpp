#include "world/StructureManager.h"
#include <cmath>
#include <algorithm>

StructureManager::StructureManager() {}

void StructureManager::update(float dt, sim::SimulationRegistry& registry) {}

void StructureManager::draw(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    // 1. Draw Settlement Footprints (Trampled Ground, mats, stone perimeters)
    for (auto& pair : registry.getAllVillages()) {
        const sim::VillageData& v = pair.second;
        if (v.centerX + 500.f < viewBounds.left || v.centerX - 500.f > viewBounds.left + viewBounds.width) continue;
        float groundY = world->getTerrainHeight(v.centerX);
        drawSettlementFootprint(target, v, groundY);
    }

    // 2. Draw Physical Base Structures
    for (auto& pair : registry.getAllStructures()) {
        sim::StructureData& s = pair.second;
        if (s.worldX < viewBounds.left - 300.f || s.worldX > viewBounds.left + viewBounds.width + 300.f) continue;

        float groundY = world->getTerrainHeight(s.worldX);
        sim::VillageData* v = registry.getVillage(s.villageId);
        sim::VillageData defaultVillage;
        if (!v) v = &defaultVillage;

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
    float footprintW = 750.f;
    float startX = village.centerX - footprintW / 2.f;

    // Trampled Earth Bed
    sf::RectangleShape dirtBed(sf::Vector2f(footprintW, 14.f));
    dirtBed.setPosition(startX, groundY - 2.f);
    dirtBed.setFillColor(sf::Color(48, 34, 22, 210));
    target.draw(dirtBed);

    // Woven reed pathway in settlement center
    sf::RectangleShape mat(sf::Vector2f(280.f, 6.f));
    mat.setOrigin(140.f, 0.f);
    mat.setPosition(village.centerX, groundY);
    mat.setFillColor(sf::Color(145, 115, 65, 190));
    target.draw(mat);

    // Decorative Ground Stones
    for (int i = -3; i <= 3; ++i) {
        if (i == 0) continue;
        sf::CircleShape stone(4.f + std::abs(i), 5);
        stone.setOrigin(stone.getRadius(), stone.getRadius());
        stone.setPosition(village.centerX + i * 90.f, groundY - 2.f);
        stone.setFillColor(sf::Color(90, 85, 78));
        target.draw(stone);
    }
}

void StructureManager::drawVillageCenter(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    sf::Color woodColor(82, 54, 30);
    sf::Color roofColor(135, 102, 55);
    sf::Color bannerColor(180, 50, 40);

    if (village.identity == sim::VillageIdentity::StoneFocused) {
        woodColor = sf::Color(65, 60, 55);
        roofColor = sf::Color(110, 100, 90);
        bannerColor = sf::Color(90, 130, 160);
    } else if (village.identity == sim::VillageIdentity::FoodRich || village.identity == sim::VillageIdentity::Peaceful) {
        roofColor = sf::Color(90, 125, 45);
        bannerColor = sf::Color(220, 175, 50);
    } else if (village.identity == sim::VillageIdentity::Aggressive) {
        bannerColor = sf::Color(190, 30, 20);
    }

    // 1. Massive Timber Support Columns
    sf::RectangleShape leftPillar(sf::Vector2f(16.f, 130.f));
    leftPillar.setOrigin(8.f, 130.f);
    leftPillar.setPosition(s.worldX - 70.f, groundY);
    leftPillar.setFillColor(woodColor);
    leftPillar.setOutlineColor(sf::Color::Black);
    leftPillar.setOutlineThickness(1.5f);
    target.draw(leftPillar);

    sf::RectangleShape rightPillar(sf::Vector2f(16.f, 130.f));
    rightPillar.setOrigin(8.f, 130.f);
    rightPillar.setPosition(s.worldX + 70.f, groundY);
    rightPillar.setFillColor(woodColor);
    rightPillar.setOutlineColor(sf::Color::Black);
    rightPillar.setOutlineThickness(1.5f);
    target.draw(rightPillar);

    sf::RectangleShape centerBeam(sf::Vector2f(190.f, 14.f));
    centerBeam.setOrigin(95.f, 14.f);
    centerBeam.setPosition(s.worldX, groundY - 105.f);
    centerBeam.setFillColor(woodColor);
    centerBeam.setOutlineColor(sf::Color::Black);
    centerBeam.setOutlineThickness(1.5f);
    target.draw(centerBeam);

    // 2. Thatched Great Canopy Roof
    sf::ConvexShape roof(4);
    roof.setPoint(0, sf::Vector2f(-110.f, 0.f));
    roof.setPoint(1, sf::Vector2f(0.f, -55.f));
    roof.setPoint(2, sf::Vector2f(110.f, 0.f));
    roof.setPoint(3, sf::Vector2f(0.f, -25.f));
    roof.setPosition(s.worldX, groundY - 115.f);
    roof.setFillColor(roofColor);
    roof.setOutlineColor(sf::Color(40, 25, 12));
    roof.setOutlineThickness(2.f);
    target.draw(roof);

    // 3. Elevated Alpha Throne Perch
    sf::RectangleShape throne(sf::Vector2f(60.f, 12.f));
    throne.setOrigin(30.f, 12.f);
    throne.setPosition(s.worldX, groundY - 35.f);
    throne.setFillColor(sf::Color(105, 75, 45));
    throne.setOutlineColor(sf::Color::Black);
    throne.setOutlineThickness(1.f);
    target.draw(throne);

    sf::RectangleShape throneBack(sf::Vector2f(12.f, 40.f));
    throneBack.setOrigin(6.f, 40.f);
    throneBack.setPosition(s.worldX - 22.f, groundY - 35.f);
    throneBack.setFillColor(sf::Color(75, 50, 30));
    target.draw(throneBack);

    // 4. Clan Totem / Tribal Banners
    sf::RectangleShape banner(sf::Vector2f(28.f, 70.f));
    banner.setOrigin(14.f, 0.f);
    banner.setPosition(s.worldX, groundY - 110.f);
    banner.setFillColor(bannerColor);
    banner.setOutlineColor(sf::Color(30, 20, 10));
    banner.setOutlineThickness(1.5f);
    target.draw(banner);

    sf::CircleShape skull(9.f, 6);
    skull.setOrigin(9.f, 9.f);
    skull.setPosition(s.worldX, groundY - 95.f);
    skull.setFillColor(sf::Color(230, 225, 210));
    skull.setOutlineColor(sf::Color::Black);
    skull.setOutlineThickness(1.f);
    target.draw(skull);
}

void StructureManager::drawNest(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    // Sturdy stilts
    sf::RectangleShape stilt1(sf::Vector2f(8.f, 35.f));
    stilt1.setOrigin(4.f, 35.f);
    stilt1.setPosition(s.worldX - 28.f, groundY);
    stilt1.setFillColor(sf::Color(80, 55, 30));
    target.draw(stilt1);

    sf::RectangleShape stilt2(sf::Vector2f(8.f, 35.f));
    stilt2.setOrigin(4.f, 35.f);
    stilt2.setPosition(s.worldX + 28.f, groundY);
    stilt2.setFillColor(sf::Color(80, 55, 30));
    target.draw(stilt2);

    // Woven Base Nest
    sf::CircleShape outer(36.f);
    outer.setOrigin(36.f, 36.f);
    outer.setPosition(s.worldX, groundY - 25.f);
    outer.setScale(1.35f, 0.45f);
    outer.setFillColor(sf::Color(170, 130, 50));
    outer.setOutlineColor(sf::Color(70, 48, 20));
    outer.setOutlineThickness(2.f);
    target.draw(outer);

    sf::CircleShape inner(24.f);
    inner.setOrigin(24.f, 24.f);
    inner.setPosition(s.worldX, groundY - 27.f);
    inner.setScale(1.25f, 0.4f);
    inner.setFillColor(sf::Color(130, 95, 35));
    target.draw(inner);

    // Leaf bedding lining
    sf::CircleShape leaves(14.f);
    leaves.setOrigin(14.f, 14.f);
    leaves.setPosition(s.worldX, groundY - 28.f);
    leaves.setScale(1.2f, 0.35f);
    leaves.setFillColor(sf::Color(75, 115, 45));
    target.draw(leaves);
}

void StructureManager::drawStorageHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    // Log Foundation
    sf::RectangleShape base(sf::Vector2f(110.f, 55.f));
    base.setOrigin(55.f, 55.f);
    base.setPosition(s.worldX, groundY);
    base.setFillColor(sf::Color(90, 60, 32));
    base.setOutlineColor(sf::Color(35, 20, 10));
    base.setOutlineThickness(1.5f);
    target.draw(base);

    // Slat wood doors
    sf::RectangleShape door(sf::Vector2f(28.f, 40.f));
    door.setOrigin(14.f, 40.f);
    door.setPosition(s.worldX, groundY);
    door.setFillColor(sf::Color(45, 28, 14));
    target.draw(door);

    // Thatched Overhanging Roof
    sf::ConvexShape roof(3);
    roof.setPoint(0, sf::Vector2f(0.f, -38.f));
    roof.setPoint(1, sf::Vector2f(-70.f, 0.f));
    roof.setPoint(2, sf::Vector2f(70.f, 0.f));
    roof.setPosition(s.worldX, groundY - 55.f);
    roof.setFillColor(sf::Color(140, 105, 50));
    roof.setOutlineColor(sf::Color(50, 35, 18));
    roof.setOutlineThickness(2.f);
    target.draw(roof);

    // Hanging Fruit / Provision Baskets outside
    for (int i = -1; i <= 1; i += 2) {
        sf::CircleShape basket(8.f);
        basket.setOrigin(8.f, 8.f);
        basket.setPosition(s.worldX + i * 42.f, groundY - 12.f);
        basket.setFillColor(sf::Color(180, 130, 45));
        target.draw(basket);

        sf::CircleShape fruit(4.f);
        fruit.setOrigin(4.f, 4.f);
        fruit.setPosition(s.worldX + i * 42.f, groundY - 15.f);
        fruit.setFillColor(sf::Color(220, 80, 40));
        target.draw(fruit);
    }
}

void StructureManager::drawWatchPlatform(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    // Twin Heavy Uprights
    sf::RectangleShape p1(sf::Vector2f(10.f, 160.f));
    p1.setOrigin(5.f, 160.f);
    p1.setPosition(s.worldX - 25.f, groundY);
    p1.setFillColor(sf::Color(75, 50, 28));
    p1.setOutlineColor(sf::Color::Black);
    p1.setOutlineThickness(1.f);
    target.draw(p1);

    sf::RectangleShape p2(sf::Vector2f(10.f, 160.f));
    p2.setOrigin(5.f, 160.f);
    p2.setPosition(s.worldX + 25.f, groundY);
    p2.setFillColor(sf::Color(75, 50, 28));
    p2.setOutlineColor(sf::Color::Black);
    p2.setOutlineThickness(1.f);
    target.draw(p2);

    // Cross Braces & Ladder rungs
    for (float y = groundY - 30.f; y > groundY - 140.f; y -= 24.f) {
        sf::RectangleShape rung(sf::Vector2f(50.f, 4.f));
        rung.setOrigin(25.f, 2.f);
        rung.setPosition(s.worldX, y);
        rung.setFillColor(sf::Color(120, 85, 45));
        target.draw(rung);
    }

    // Elevated Lookout Floor
    sf::RectangleShape platform(sf::Vector2f(80.f, 10.f));
    platform.setOrigin(40.f, 10.f);
    platform.setPosition(s.worldX, groundY - 145.f);
    platform.setFillColor(sf::Color(110, 75, 40));
    platform.setOutlineColor(sf::Color::Black);
    platform.setOutlineThickness(1.5f);
    target.draw(platform);

    // Guard Railing
    sf::RectangleShape rail(sf::Vector2f(80.f, 4.f));
    rail.setOrigin(40.f, 4.f);
    rail.setPosition(s.worldX, groundY - 165.f);
    rail.setFillColor(sf::Color(140, 100, 55));
    target.draw(rail);
}

void StructureManager::drawBuilderHut(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    // Open-sided Workshop Canopy
    sf::RectangleShape postL(sf::Vector2f(10.f, 65.f));
    postL.setOrigin(5.f, 65.f);
    postL.setPosition(s.worldX - 45.f, groundY);
    postL.setFillColor(sf::Color(80, 52, 28));
    target.draw(postL);

    sf::RectangleShape postR(sf::Vector2f(10.f, 65.f));
    postR.setOrigin(5.f, 65.f);
    postR.setPosition(s.worldX + 45.f, groundY);
    postR.setFillColor(sf::Color(80, 52, 28));
    target.draw(postR);

    // Slanted Toolmaker Roof
    sf::RectangleShape roof(sf::Vector2f(110.f, 12.f));
    roof.setOrigin(55.f, 12.f);
    roof.setPosition(s.worldX, groundY - 60.f);
    roof.setRotation(-4.f);
    roof.setFillColor(sf::Color(125, 90, 48));
    roof.setOutlineColor(sf::Color(40, 25, 12));
    roof.setOutlineThickness(1.5f);
    target.draw(roof);

    // Workbench with Anvil Stone
    sf::RectangleShape bench(sf::Vector2f(50.f, 16.f));
    bench.setOrigin(25.f, 16.f);
    bench.setPosition(s.worldX, groundY);
    bench.setFillColor(sf::Color(105, 75, 42));
    target.draw(bench);

    sf::CircleShape anvil(8.f, 6);
    anvil.setOrigin(8.f, 8.f);
    anvil.setPosition(s.worldX + 12.f, groundY - 18.f);
    anvil.setFillColor(sf::Color(100, 100, 100));
    target.draw(anvil);
}

void StructureManager::drawBonfire(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    // Stone Hearth Circle
    sf::CircleShape stoneRing(26.f, 8);
    stoneRing.setOrigin(26.f, 26.f);
    stoneRing.setPosition(s.worldX, groundY - 2.f);
    stoneRing.setScale(1.4f, 0.45f);
    stoneRing.setFillColor(sf::Color(75, 70, 65));
    stoneRing.setOutlineColor(sf::Color(30, 30, 30));
    stoneRing.setOutlineThickness(2.f);
    target.draw(stoneRing);

    // Crossed Charred Logs
    sf::RectangleShape log1(sf::Vector2f(34.f, 8.f));
    log1.setOrigin(17.f, 4.f);
    log1.setPosition(s.worldX, groundY - 6.f);
    log1.setRotation(18.f);
    log1.setFillColor(sf::Color(55, 32, 16));
    target.draw(log1);

    sf::RectangleShape log2(sf::Vector2f(34.f, 8.f));
    log2.setOrigin(17.f, 4.f);
    log2.setPosition(s.worldX, groundY - 6.f);
    log2.setRotation(-22.f);
    log2.setFillColor(sf::Color(45, 25, 12));
    target.draw(log2);

    // Roaring Hearth Fire
    sf::ConvexShape fireOuter(3);
    fireOuter.setPoint(0, sf::Vector2f(0.f, -32.f));
    fireOuter.setPoint(1, sf::Vector2f(14.f, 0.f));
    fireOuter.setPoint(2, sf::Vector2f(-14.f, 0.f));
    fireOuter.setPosition(s.worldX, groundY - 8.f);
    fireOuter.setFillColor(sf::Color(235, 90, 20, 220));
    target.draw(fireOuter);

    sf::ConvexShape fireInner(3);
    fireInner.setPoint(0, sf::Vector2f(0.f, -18.f));
    fireInner.setPoint(1, sf::Vector2f(8.f, 0.f));
    fireInner.setPoint(2, sf::Vector2f(-8.f, 0.f));
    fireInner.setPosition(s.worldX, groundY - 8.f);
    fireInner.setFillColor(sf::Color(255, 215, 60, 240));
    target.draw(fireInner);
}

void StructureManager::drawSimpleBarrier(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    // Spiked Palisade Stakes
    for (int i = -2; i <= 2; ++i) {
        float stakeH = 50.f + std::abs(i) * 6.f;
        sf::ConvexShape stake(3);
        stake.setPoint(0, sf::Vector2f(0.f, -stakeH));
        stake.setPoint(1, sf::Vector2f(6.f, 0.f));
        stake.setPoint(2, sf::Vector2f(-6.f, 0.f));
        stake.setPosition(s.worldX + i * 14.f, groundY);
        stake.setFillColor(sf::Color(95, 65, 36));
        stake.setOutlineColor(sf::Color(35, 20, 10));
        stake.setOutlineThickness(1.f);
        target.draw(stake);
    }
}

void StructureManager::drawStockpileProps(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    if (s.type == sim::StructureType::WoodPile) {
        // Stacked Timber Logs
        for (int row = 0; row < 3; ++row) {
            int count = 4 - row;
            for (int i = 0; i < count; ++i) {
                sf::CircleShape logEnd(6.f);
                logEnd.setOrigin(6.f, 6.f);
                logEnd.setPosition(s.worldX - 18.f + i * 12.f + row * 6.f, groundY - 6.f - row * 10.f);
                logEnd.setFillColor(sf::Color(165, 125, 75));
                logEnd.setOutlineColor(sf::Color(65, 42, 20));
                logEnd.setOutlineThickness(1.5f);
                target.draw(logEnd);
            }
        }
    } else {
        // Cut Stone Blocks & Flint Nodules
        for (int i = -1; i <= 1; ++i) {
            sf::RectangleShape block(sf::Vector2f(18.f, 14.f));
            block.setOrigin(9.f, 14.f);
            block.setPosition(s.worldX + i * 16.f, groundY);
            block.setFillColor(sf::Color(120, 115, 110));
            block.setOutlineColor(sf::Color(45, 42, 40));
            block.setOutlineThickness(1.5f);
            target.draw(block);
        }
    }
}

void StructureManager::drawConstructionSite(sf::RenderTarget& target, const sim::StructureData& s, float groundY) {
    float progressRatio = std::clamp(s.progress / std::max(1.f, s.maxProgress), 0.f, 1.f);

    // Ground Stake Boundary
    sf::RectangleShape stakeL(sf::Vector2f(6.f, 28.f));
    stakeL.setOrigin(3.f, 28.f);
    stakeL.setPosition(s.worldX - 40.f, groundY);
    stakeL.setFillColor(sf::Color(90, 60, 30));
    target.draw(stakeL);

    sf::RectangleShape stakeR(sf::Vector2f(6.f, 28.f));
    stakeR.setOrigin(3.f, 28.f);
    stakeR.setPosition(s.worldX + 40.f, groundY);
    stakeR.setFillColor(sf::Color(90, 60, 30));
    target.draw(stakeR);

    // Guide Cord
    sf::RectangleShape cord(sf::Vector2f(80.f, 2.f));
    cord.setOrigin(40.f, 1.f);
    cord.setPosition(s.worldX, groundY - 20.f);
    cord.setFillColor(sf::Color(210, 180, 90));
    target.draw(cord);

    // Scaffolding Uprights
    if (progressRatio >= 0.33f) {
        sf::RectangleShape scaf(sf::Vector2f(60.f, 35.f));
        scaf.setOrigin(30.f, 35.f);
        scaf.setPosition(s.worldX, groundY);
        scaf.setFillColor(sf::Color::Transparent);
        scaf.setOutlineColor(sf::Color(145, 95, 45));
        scaf.setOutlineThickness(2.f);
        target.draw(scaf);
    }
}