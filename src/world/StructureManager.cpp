#include "world/StructureManager.h"
#include "world/WorldManager.h"
#include <cmath>
#include <algorithm>

StructureManager::StructureManager() {}

void StructureManager::setTexture(const sf::Texture& tex) {
    villageTexture = &tex;
}

void StructureManager::update(float dt, sim::SimulationRegistry& registry) {
    if (activeBuilderId == 0) {
        for (auto& pair : registry.getAllApes()) {
            sim::ApeData& ape = pair.second;
            if (!ape.alive || ape.id == registry.getControlledApe()) continue;
            activeBuilderId = ape.id;
            ape.depthLane = sim::DepthLane::Foreground;
            ape.currentJob = sim::Job::Builder;
            break;
        }
    }

    if (activeBuilderId != 0) {
        sim::ApeData* builder = registry.getApe(activeBuilderId);
        if (builder && builder->alive) {
            builder->depthLane = sim::DepthLane::Foreground;
            builder->currentJob = sim::Job::Builder;
        }
    }

    updateUpgrades(dt, registry);
}

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
    if (!rearLawnTexture || rearLawnTexture->getSize().x == 0) return;

    sf::View view = target.getView();
    float viewLeft = view.getCenter().x - view.getSize().x * 0.5f - 400.f;
    float viewWidth = view.getSize().x + 800.f;

    float yardTopY = groundY - 228.f;
    float yardBottomY = groundY - 14.f;
    float yardH = yardBottomY - yardTopY;

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
}

void StructureManager::drawRearPalisade(sf::RenderTarget& target, const sim::VillageData& village, float groundY) {
    if (!villageTexture || villageTexture->getSize().x == 0) return;

    sf::View view = target.getView();
    float viewLeft = view.getCenter().x - view.getSize().x * 0.5f - 100.f;
    float viewRight = view.getCenter().x + view.getSize().x * 0.5f + 100.f;

    float drawStart = std::max(viewLeft, village.borderMinX);
    float drawEnd = std::min(viewRight, village.borderMaxX);
    if (drawStart >= drawEnd) return;

    float rearFenceBaseY = groundY - 223.f;
    float fenceScale = 0.30f;
    float stepW = static_cast<float>(rectPalisadeMiddle.width) * fenceScale * 0.96f;
    sf::Color fenceBgColor(180, 185, 200, 245);

    float startX = std::floor(drawStart / stepW) * stepW;
    for (float fx = startX; fx < drawEnd; fx += stepW) {
        drawSpriteAnchored(target, rectPalisadeMiddle, fx, rearFenceBaseY, fenceScale, fenceBgColor);
    }
}

void StructureManager::drawMiddlePalisade(sf::RenderTarget& target, const sim::VillageData& village, float groundY) {
    if (!villageTexture || villageTexture->getSize().x == 0) return;

    sf::View view = target.getView();
    float viewLeft = view.getCenter().x - view.getSize().x * 0.5f - 100.f;
    float viewRight = view.getCenter().x + view.getSize().x * 0.5f + 100.f;

    float drawStart = std::max(viewLeft, village.borderMinX);
    float drawEnd = std::min(viewRight, village.borderMaxX);
    if (drawStart >= drawEnd) return;

    float midFenceBaseY = groundY - 14.f;
    float fenceScale = 0.58f;
    float stepW = static_cast<float>(rectPalisadeMiddle.width) * fenceScale * 0.96f;
    sf::Color fenceColor(230, 230, 235, 255);

    float lodgeHalfWidth = 240.f;
    float startX = std::floor(drawStart / stepW) * stepW;

    for (float fx = startX; fx < drawEnd; fx += stepW) {
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
    drawRearPalisade(target, village, groundY);
    drawMiddlePalisade(target, village, groundY);
}

void StructureManager::drawBackgroundStructures(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    float defaultGroundY = world ? world->getTerrainHeight(viewBounds.left + viewBounds.width * 0.5f) : 500.f;
    sim::VillageData dummy;
    drawRearLawn(target, dummy, defaultGroundY);
    drawFrontRoad(target, dummy, defaultGroundY);

    for (const auto& pair : registry.getAllVillages()) {
        const sim::VillageData& village = pair.second;
        if (village.borderMaxX < viewBounds.left || village.borderMinX > viewBounds.left + viewBounds.width) {
            continue;
        }
        float groundY = world ? world->getTerrainHeight(village.centerX) : 500.f;
        drawRearPalisade(target, village, groundY);
    }
}

void StructureManager::drawMidgroundStructures(sf::RenderTarget& target, sim::SimulationRegistry& registry, WorldManager* world, const sf::FloatRect& viewBounds) {
    for (const auto& pair : registry.getAllVillages()) {
        const sim::VillageData& village = pair.second;
        if (village.borderMaxX < viewBounds.left || village.borderMinX > viewBounds.left + viewBounds.width) {
            continue;
        }

        float groundY = world ? world->getTerrainHeight(village.centerX) : 500.f;
        drawMiddlePalisade(target, village, groundY);

        for (const auto& spair : registry.getAllStructures()) {
            const sim::StructureData& s = spair.second;
            if (s.villageId != village.id) continue;
            if (s.worldX < viewBounds.left - 400.f || s.worldX > viewBounds.left + viewBounds.width + 400.f) continue;

            if (s.type == sim::StructureType::ToolRack) {
                drawToolRack(target, s, village, groundY);
            } else if (s.type == sim::StructureType::StonePile || s.type == sim::StructureType::WoodPile) {
                drawStockpileProps(target, s, village, groundY);
            } else if (s.type == sim::StructureType::WatchPlatform) {
                drawWatchPlatform(target, s, village, groundY);
            } else if (s.type == sim::StructureType::VillageCenter) {
                drawVillageCenter(target, s, village, groundY);
            } else if (s.type == sim::StructureType::EmptyPlot) {
                drawEmptyPlot(target, s, groundY);
            }
        }
    }
}

bool StructureManager::tryStartUpgrade(sim::VillageData& village, sim::SimulationRegistry& registry) {
    if (upgradePhase != VillageUpgradePhase::Idle) return false;
    if (village.amber < upgradeCost) return false;

    village.amber -= upgradeCost;
    upgradePhase = VillageUpgradePhase::Sinking;
    upgradeTimer = 0.f;
    buildProgress = 0.f;
    upgradeModalOpen = false;

    activeBuilderId = 0;
    if (village.chiefBuilderId != 0 && village.chiefBuilderId != registry.getControlledApe()) {
        activeBuilderId = village.chiefBuilderId;
    }

    if (activeBuilderId == 0) {
        for (sim::EntityID mId : village.members) {
            if (mId != 0 && mId != registry.getControlledApe()) {
                activeBuilderId = mId;
                break;
            }
        }
    }

    if (activeBuilderId == 0) {
        for (auto& pair : registry.getAllApes()) {
            if (pair.first != registry.getControlledApe() && pair.second.alive) {
                activeBuilderId = pair.first;
                break;
            }
        }
    }

    sim::ApeData* builder = registry.getApe(activeBuilderId);
    if (builder) {
        builder->depthLane = sim::DepthLane::Foreground;
        builder->currentJob = sim::Job::Builder;
        builder->hasTravelDestination = true;
        builder->travelDestinationX = village.centerX + 70.f;
    }

    return true;
}

void StructureManager::updateUpgrades(float dt, sim::SimulationRegistry& registry) {
    if (upgradePhase == VillageUpgradePhase::Idle) return;

    upgradeTimer += dt;

    if (upgradePhase == VillageUpgradePhase::Sinking) {
        sim::ApeData* builder = registry.getApe(activeBuilderId);
        if (builder && builder->alive) {
            builder->depthLane = sim::DepthLane::Foreground;
            builder->currentJob = sim::Job::Builder;
        }

        if (upgradeTimer >= 2.0f) {
            upgradePhase = VillageUpgradePhase::WaitingForBuilder;
            upgradeTimer = 0.f;
            buildProgress = 0.f;
        }
    } else if (upgradePhase == VillageUpgradePhase::WaitingForBuilder) {
        sim::ApeData* builder = registry.getApe(activeBuilderId);
        if (!builder || !builder->alive) {
            for (auto& pair : registry.getAllApes()) {
                if (pair.first != registry.getControlledApe() && pair.second.alive) {
                    activeBuilderId = pair.first;
                    builder = &pair.second;
                    break;
                }
            }
        }

        if (builder) {
            builder->depthLane = sim::DepthLane::Foreground;
            builder->currentJob = sim::Job::Builder;

            sim::VillageData* village = registry.getVillage(builder->villageId);
            float targetX = village ? (village->centerX + 70.f) : (builder->worldX);

            float dist = std::abs(builder->worldX - targetX);
            if (dist > 40.f) {
                builder->hasTravelDestination = true;
                builder->travelDestinationX = targetX;
            } else {
                builder->hasTravelDestination = false;
                upgradePhase = VillageUpgradePhase::Building;
                upgradeTimer = 0.f;
                buildProgress = 0.f;
            }
        }
    } else if (upgradePhase == VillageUpgradePhase::Building) {
        sim::ApeData* builder = registry.getApe(activeBuilderId);
        if (builder && builder->alive) {
            builder->depthLane = sim::DepthLane::Foreground;
            builder->currentJob = sim::Job::Builder;
            builder->hasTravelDestination = false;

            buildProgress += (dt / totalBuildDuration);
            if (buildProgress >= 1.0f) {
                buildProgress = 1.0f;
                upgradePhase = VillageUpgradePhase::Rising;
                upgradeTimer = 0.f;
            }
        } else {
            upgradePhase = VillageUpgradePhase::WaitingForBuilder;
        }
    } else if (upgradePhase == VillageUpgradePhase::Rising) {
        if (upgradeTimer >= 2.5f) {
            upgradePhase = VillageUpgradePhase::Idle;
            upgradeTimer = 0.f;
            buildProgress = 0.f;
            upgradeCost = static_cast<int>(upgradeCost * 2.0f);

            for (auto& pair : registry.getAllVillages()) {
                sim::VillageData& v = pair.second;
                if (v.tier == sim::SettlementTier::FirePit) {
                    v.tier = sim::SettlementTier::Camp;
                } else if (v.tier == sim::SettlementTier::Camp) {
                    v.tier = sim::SettlementTier::Village;
                }
            }

            sim::ApeData* builder = registry.getApe(activeBuilderId);
            if (builder && builder->alive) {
                builder->depthLane = sim::DepthLane::Foreground;
                builder->currentJob = sim::Job::Builder;
            }
        }
    }
}

void StructureManager::drawVillageCenter(sf::RenderTarget& target, const sim::StructureData& s, const sim::VillageData& village, float groundY) {
    if (upgradePhase == VillageUpgradePhase::Sinking) {
        float p = std::clamp(upgradeTimer / 2.0f, 0.0f, 1.0f);
        float ease = p * p;

        if (village.tier == sim::SettlementTier::FirePit) {
            float sinkY = groundY + (ease * 110.f);
            drawSpriteAnchored(target, rectFirePit, s.worldX, sinkY, 1.0f);
            drawSpriteAnchored(target, rectFxFire, s.worldX, sinkY - 4.f, std::max(0.0f, 1.0f - ease));
        } else {
            float sinkY = groundY + (ease * 480.f);
            drawSpriteAnchored(target, tier1Visual.spriteRect, s.worldX, sinkY, tier1Visual.scale);
        }
        return;
    }

    if (upgradePhase == VillageUpgradePhase::WaitingForBuilder) {
        float scaffoldH = 75.f;
        int posts = 7;
        float startX = s.worldX - 220.f;
        float spacing = 440.f / static_cast<float>(posts - 1);

        for (int i = 0; i < posts; ++i) {
            float px = startX + i * spacing;
            sf::RectangleShape post(sf::Vector2f(12.f, scaffoldH));
            post.setOrigin(6.f, scaffoldH);
            post.setPosition(px, groundY);
            post.setFillColor(sf::Color(108, 72, 38));
            post.setOutlineColor(sf::Color(32, 18, 8));
            post.setOutlineThickness(2.f);
            target.draw(post);
        }

        sf::RectangleShape plank(sf::Vector2f(470.f, 11.f));
        plank.setOrigin(235.f, 5.5f);
        plank.setPosition(s.worldX, groundY - 35.f);
        plank.setFillColor(sf::Color(138, 96, 52));
        plank.setOutlineColor(sf::Color(32, 18, 8));
        plank.setOutlineThickness(1.5f);
        target.draw(plank);
        return;
    }

    if (upgradePhase == VillageUpgradePhase::Building) {
        float p = std::clamp(buildProgress, 0.0f, 1.0f);
        float scaffoldH = 75.f + p * 320.f;

        int posts = 7;
        float startX = s.worldX - 220.f;
        float spacing = 440.f / static_cast<float>(posts - 1);

        for (int i = 0; i < posts; ++i) {
            float px = startX + i * spacing;
            sf::RectangleShape post(sf::Vector2f(12.f, scaffoldH));
            post.setOrigin(6.f, scaffoldH);
            post.setPosition(px, groundY);
            post.setFillColor(sf::Color(108, 72, 38));
            post.setOutlineColor(sf::Color(32, 18, 8));
            post.setOutlineThickness(2.f);
            target.draw(post);
        }

        int planks = std::max(1, static_cast<int>(1 + p * 6.0f));
        for (int i = 1; i <= planks; ++i) {
            float py = groundY - (i * 48.f);
            sf::RectangleShape plank(sf::Vector2f(470.f, 11.f));
            plank.setOrigin(235.f, 5.5f);
            plank.setPosition(s.worldX, py);
            plank.setFillColor(sf::Color(138, 96, 52));
            plank.setOutlineColor(sf::Color(32, 18, 8));
            plank.setOutlineThickness(1.5f);
            target.draw(plank);

            for (int j = 0; j < posts - 1; ++j) {
                float x1 = startX + j * spacing;
                float x2 = startX + (j + 1) * spacing;
                sf::Vertex brace[] = {
                    sf::Vertex(sf::Vector2f(x1, py), sf::Color(84, 54, 26)),
                    sf::Vertex(sf::Vector2f(x2, py + 48.f), sf::Color(84, 54, 26)),
                    sf::Vertex(sf::Vector2f(x2, py), sf::Color(84, 54, 26)),
                    sf::Vertex(sf::Vector2f(x1, py + 48.f), sf::Color(84, 54, 26))
                };
                target.draw(brace, 4, sf::Lines);
            }
        }
        return;
    }

    if (upgradePhase == VillageUpgradePhase::Rising) {
        float p = std::clamp(upgradeTimer / 2.5f, 0.0f, 1.0f);
        float ease = 1.0f - std::pow(1.0f - p, 3.0f);
        float riseY = groundY + ((1.0f - ease) * 480.f);

        if (village.tier == sim::SettlementTier::FirePit) {
            drawSpriteAnchored(target, tier1Visual.spriteRect, s.worldX, riseY, tier1Visual.scale);
        } else {
            drawSpriteAnchored(target, tier2Visual.spriteRect, s.worldX, riseY, tier2Visual.scale);
        }
        return;
    }

    if (village.tier == sim::SettlementTier::FirePit) {
        drawSpriteAnchored(target, rectFirePit, s.worldX, groundY, 1.0f);
        drawSpriteAnchored(target, rectFxFire, s.worldX, groundY - 4.f, 1.0f);
        return;
    }

    if (villageTexture && villageTexture->getSize().x > 0) {
        const BuildingTierVisual& visual = (village.tier == sim::SettlementTier::Camp) ? tier1Visual : tier2Visual;
        drawSpriteAnchored(target, visual.spriteRect, s.worldX, groundY, visual.scale);
        drawSpriteAnchored(target, rectFxFire, s.worldX - 44.f, groundY - 2.f, 0.88f);
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
                    skullTop.setPosition(mover->worldX + (v.expandingSideRight ? 26.f : -26.f), groundY - 75.f);
                    skullTop.setFillColor(sf::Color(220, 180, 80));
                    skullTop.setOutlineColor(sf::Color(24, 14, 6));
                    skullTop.setOutlineThickness(2.f);
                    target.draw(skullTop);
                }
            }
        }
    }
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

void StructureManager::setTier2Visual(const std::string& textureKey, const sf::IntRect& rect, float scale) {
    tier2Visual.textureKey = textureKey;
    tier2Visual.spriteRect = rect;
    tier2Visual.scale = scale;
}

bool StructureManager::handleModalClick(const sf::Vector2f& uiCoords, sim::VillageData& village, sim::SimulationRegistry& registry) {
    if (!upgradeModalOpen) return false;

    if (modalCloseButtonBounds.contains(uiCoords)) {
        upgradeModalOpen = false;
        return true;
    }

    if (modalUpgradeButtonBounds.contains(uiCoords)) {
        return tryStartUpgrade(village, registry);
    }

    return false;
}

void StructureManager::drawUpgradeModal(sf::RenderTarget& target, const sf::Font& font, int villageAmber, sim::SettlementTier tier) {
    if (!upgradeModalOpen) return;

    sf::RectangleShape dim(sf::Vector2f(1280.f, 720.f));
    dim.setFillColor(sf::Color(0, 0, 0, 160));
    target.draw(dim);

    float panelW = 460.f;
    float panelH = 300.f;
    float panelX = (1280.f - panelW) * 0.5f;
    float panelY = (720.f - panelH) * 0.5f;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(panelX, panelY);
    panel.setFillColor(sf::Color(22, 16, 12, 245));
    panel.setOutlineColor(sf::Color(180, 140, 60));
    panel.setOutlineThickness(3.f);
    target.draw(panel);

    sf::Text title("VILLAGE UPGRADE", font, 22);
    title.setFillColor(sf::Color(255, 215, 90));
    title.setOutlineColor(sf::Color::Black);
    title.setOutlineThickness(1.5f);
    sf::FloatRect tb = title.getLocalBounds();
    title.setPosition(panelX + (panelW - tb.width) * 0.5f, panelY + 24.f);
    target.draw(title);

    std::string currentStr = (tier == sim::SettlementTier::FirePit) ? "Campfire" : "Hut Encampment";
    std::string nextStr = (tier == sim::SettlementTier::FirePit) ? "Hut Encampment" : "Village Compound";
    sf::Text desc("Current: " + currentStr + "\nNext:    " + nextStr, font, 16);
    desc.setFillColor(sf::Color(220, 210, 195));
    desc.setOutlineColor(sf::Color::Black);
    desc.setOutlineThickness(1.f);
    desc.setPosition(panelX + 44.f, panelY + 85.f);
    target.draw(desc);

    bool canAfford = (villageAmber >= upgradeCost);
    float btnW = 280.f;
    float btnH = 46.f;
    float btnX = panelX + (panelW - btnW) * 0.5f;
    float btnY = panelY + 165.f;
    modalUpgradeButtonBounds = sf::FloatRect(btnX, btnY, btnW, btnH);

    sf::RectangleShape btn(sf::Vector2f(btnW, btnH));
    btn.setPosition(btnX, btnY);
    btn.setFillColor(canAfford ? sf::Color(55, 40, 20) : sf::Color(35, 25, 22));
    btn.setOutlineColor(canAfford ? sf::Color(240, 185, 50) : sf::Color(110, 50, 40));
    btn.setOutlineThickness(2.f);
    target.draw(btn);

    sf::CircleShape amberGem(6.f, 4);
    amberGem.setOrigin(6.f, 6.f);
    amberGem.setPosition(btnX + 22.f, btnY + 23.f);
    amberGem.setFillColor(sf::Color(255, 185, 25));
    target.draw(amberGem);

    std::string btnStr = "Upgrade (" + std::to_string(upgradeCost) + " Amber)";
    sf::Text btnText(btnStr, font, 15);
    btnText.setFillColor(canAfford ? sf::Color(255, 235, 180) : sf::Color(170, 90, 80));
    btnText.setOutlineColor(sf::Color::Black);
    btnText.setOutlineThickness(1.f);
    btnText.setPosition(btnX + 40.f, btnY + 13.f);
    target.draw(btnText);

    modalCloseButtonBounds = sf::FloatRect(panelX + panelW - 42.f, panelY + 12.f, 30.f, 30.f);
    sf::Text closeText("X", font, 18);
    closeText.setFillColor(sf::Color(200, 80, 80));
    closeText.setPosition(modalCloseButtonBounds.left + 8.f, modalCloseButtonBounds.top + 4.f);
    target.draw(closeText);

    sf::Text escPrompt("[ESC] Close   |   [E / Enter] Confirm", font, 13);
    escPrompt.setFillColor(sf::Color(140, 130, 120));
    escPrompt.setPosition(panelX + 44.f, panelY + 248.f);
    target.draw(escPrompt);
}