#include "entities/Tree.h"
#include "world/SeedManager.h"
#include "core/VisualConfig.h"
#include <cmath>
#include <algorithm>
#include <iostream>

float Tree::globalShadowShearX = 0.f;
float Tree::globalShadowProjY = 0.2f;
sf::Color Tree::globalShadowColor = sf::Color(10, 14, 22, 100);

void Tree::setGlobalShadowParams(float shearX, float projY, sf::Color color) {
    globalShadowShearX = shearX;
    globalShadowProjY = projY;
    globalShadowColor = color;
}

sf::Texture& Tree::getVariantTexture(int variant, sf::Texture& fallbackTex) {
    static sf::Texture textures[5];
    static bool loaded[5] = {false, false, false, false, false};

    int idx = std::clamp(variant - 1, 0, 4);
    if (!loaded[idx]) {
        std::string filenamePng = "assets/sprites/TREEFIXED_" + std::to_string(idx + 1) + ".png";
        std::string filenameJpg = "assets/sprites/TREEFIXED_" + std::to_string(idx + 1) + ".jpg";

        if (!textures[idx].loadFromFile(filenamePng)) {
            if (!textures[idx].loadFromFile(filenameJpg)) {
                return fallbackTex;
            }
        }
        textures[idx].setSmooth(false);
        loaded[idx] = true;
    }
    return textures[idx];
}

sf::Texture& Tree::getJungleGroundTexture(sf::Texture& fallbackTex) {
    static sf::Texture groundTex;
    static bool loaded = false;
    if (!loaded) {
        if (!groundTex.loadFromFile("assets/sprites/assetsfixed3.png")) {
            if (!groundTex.loadFromFile("assets/sprites/assetsfixed3.jpg")) {
                return fallbackTex;
            }
        }
        groundTex.setSmooth(false);
        loaded = true;
    }
    return groundTex;
}

void Tree::setupVariant(int variant, sf::Texture& fallbackTex) {
    variantIndex = std::clamp(variant, 1, 5);
    sf::Texture& tex = getVariantTexture(variantIndex, fallbackTex);

    sf::Vector2u texSize = tex.getSize();
    float spriteW = static_cast<float>(texSize.x);
    float spriteH = static_cast<float>(texSize.y);

    if (spriteW == 0.0f || spriteH == 0.0f) {
        if (variantIndex == 1) { spriteW = 80.f; spriteH = 200.f; }
        else if (variantIndex == 2) { spriteW = 120.f; spriteH = 260.f; }
        else if (variantIndex == 3) { spriteW = 170.f; spriteH = 330.f; }
        else if (variantIndex == 4) { spriteW = 240.f; spriteH = 410.f; }
        else { spriteW = 330.f; spriteH = 470.f; }
    }

    float trunkW = 35.0f;
    if (variantIndex == 2) trunkW = 48.0f;
    else if (variantIndex == 3) trunkW = 68.0f;
    else if (variantIndex == 4) trunkW = 95.0f;
    else if (variantIndex == 5) trunkW = 140.0f;

    trunkBounds = sf::FloatRect(worldX - trunkW * 0.5f, worldY - spriteH, trunkW, spriteH);
    totalBounds = sf::FloatRect(worldX - spriteW * 0.5f, worldY - spriteH, spriteW, spriteH);

    trunkSprite.setTexture(tex, true);
    trunkSprite.setOrigin(spriteW * 0.5f, spriteH);
    trunkSprite.setPosition(worldX, worldY);
    trunkSprite.setScale(1.0f, 1.0f);

    hasRoot = false;
    if (variantIndex >= 3) {
        sf::Texture& gTex = getJungleGroundTexture(fallbackTex);
        rootSprite.setTexture(gTex);
        sf::IntRect rootRect = VisualConfig::JUNGLE_ROOT_01;
        float rootScale = 0.55f;

        if (variantIndex == 4) {
            rootRect = VisualConfig::JUNGLE_ROOT_02;
            rootScale = 0.70f;
        } else if (variantIndex == 5) {
            rootRect = VisualConfig::JUNGLE_ROOT_03;
            rootScale = 0.90f;
        }

        rootSprite.setTextureRect(rootRect);
        rootSprite.setOrigin(rootRect.width * 0.5f, static_cast<float>(rootRect.height));
        rootSprite.setPosition(worldX, worldY + 6.0f);
        rootSprite.setScale(rootScale, rootScale);
        hasRoot = true;
    }

    branchData.clear();
    vineData.clear();

    float bHeight = 16.0f;
    if (variantIndex == 1) {
        branchData.push_back({ sf::FloatRect(worldX - 35.f, worldY - spriteH * 0.60f, 30.f, bHeight) });
    } else if (variantIndex == 2) {
        branchData.push_back({ sf::FloatRect(worldX - 45.f, worldY - spriteH * 0.50f, 40.f, bHeight) });
        branchData.push_back({ sf::FloatRect(worldX + 5.f, worldY - spriteH * 0.72f, 40.f, bHeight) });
    } else if (variantIndex == 3) {
        branchData.push_back({ sf::FloatRect(worldX - 65.f, worldY - spriteH * 0.48f, 55.f, bHeight) });
        branchData.push_back({ sf::FloatRect(worldX + 10.f, worldY - spriteH * 0.68f, 55.f, bHeight) });
        vineData.push_back({ sf::Vector2f(worldX - 25.f, worldY - spriteH * 0.65f), 75.f, 0.f });
    } else if (variantIndex == 4) {
        branchData.push_back({ sf::FloatRect(worldX - 90.f, worldY - spriteH * 0.42f, 75.f, bHeight) });
        branchData.push_back({ sf::FloatRect(worldX + 15.f, worldY - spriteH * 0.62f, 75.f, bHeight) });
        branchData.push_back({ sf::FloatRect(worldX - 70.f, worldY - spriteH * 0.80f, 60.f, bHeight) });
        vineData.push_back({ sf::Vector2f(worldX - 35.f, worldY - spriteH * 0.60f), 100.f, 0.f });
        vineData.push_back({ sf::Vector2f(worldX + 35.f, worldY - spriteH * 0.55f), 90.f, 0.f });
    } else if (variantIndex == 5) {
        branchData.push_back({ sf::FloatRect(worldX - 130.f, worldY - spriteH * 0.38f, 105.f, bHeight) });
        branchData.push_back({ sf::FloatRect(worldX + 25.f, worldY - spriteH * 0.55f, 105.f, bHeight) });
        branchData.push_back({ sf::FloatRect(worldX - 100.f, worldY - spriteH * 0.72f, 85.f, bHeight) });
        branchData.push_back({ sf::FloatRect(worldX + 20.f, worldY - spriteH * 0.82f, 80.f, bHeight) });
        vineData.push_back({ sf::Vector2f(worldX - 60.f, worldY - spriteH * 0.55f), 140.f, 0.f });
        vineData.push_back({ sf::Vector2f(worldX + 55.f, worldY - spriteH * 0.50f), 130.f, 0.f });
        vineData.push_back({ sf::Vector2f(worldX - 20.f, worldY - spriteH * 0.45f), 110.f, 0.f });
    }
}

Tree::Tree(float x, float y, int variant, sf::Texture& decorTexture, int id)
    : treeId(id), worldX(x), worldY(y) {
    setupVariant(variant, decorTexture);
}

Tree::Tree(float x, float y, float, float, sf::Color, sf::Texture& decorTexture, int id)
    : treeId(id), worldX(x), worldY(y) {
    setupVariant(1, decorTexture);
}

void Tree::appendQuad(sf::VertexArray& mesh, const sf::FloatRect& rect, sf::Color color) {
    mesh.append(sf::Vertex(sf::Vector2f(rect.left, rect.top), color));
    mesh.append(sf::Vertex(sf::Vector2f(rect.left + rect.width, rect.top), color));
    mesh.append(sf::Vertex(sf::Vector2f(rect.left, rect.top + rect.height), color));
    mesh.append(sf::Vertex(sf::Vector2f(rect.left, rect.top + rect.height), color));
    mesh.append(sf::Vertex(sf::Vector2f(rect.left + rect.width, rect.top), color));
    mesh.append(sf::Vertex(sf::Vector2f(rect.left + rect.width, rect.top + rect.height), color));
}

void Tree::appendOctagon(sf::VertexArray& mesh, const sf::Vector2f& center, float radius, sf::Color color) {
    const int segments = 8;
    for (int i = 0; i < segments; ++i) {
        float a1 = (i * 3.14159f * 2.f) / segments;
        float a2 = ((i + 1) * 3.14159f * 2.f) / segments;
        mesh.append(sf::Vertex(center, color));
        mesh.append(sf::Vertex(center + sf::Vector2f(std::cos(a1) * radius, std::sin(a1) * radius), color));
        mesh.append(sf::Vertex(center + sf::Vector2f(std::cos(a2) * radius, std::sin(a2) * radius), color));
    }
}

void Tree::addBranch(float, float, bool, sf::Color, sf::Texture&) {}

void Tree::addVine(float, float, float) {}

void Tree::buildCanopy(uint32_t&, float, float, sf::Color, int) {}

void Tree::initDynamicMesh() {}

void Tree::updateSway(float globalTime, const sf::Vector2f& windVector) {
    if (harvestState == TreeHarvestState::Falling || harvestState == TreeHarvestState::Fading || harvestState == TreeHarvestState::Harvested) {
        return;
    }

    float sway = std::sin(globalTime * 1.5f + worldX * 0.01f) * (0.35f + std::abs(windVector.x) * 0.75f);
    trunkSprite.setRotation(sway);

    for (auto& vine : vineData) {
        vine.disturbance *= 0.95f;
    }
}

void Tree::disturbVines(const sf::FloatRect& bounds, float velocityX) {
    if (harvestState == TreeHarvestState::Falling || harvestState == TreeHarvestState::Fading || harvestState == TreeHarvestState::Harvested) {
        return;
    }

    for (auto& vine : vineData) {
        sf::FloatRect vBounds(vine.origin.x - 4.f, vine.origin.y, 8.f, vine.length);
        if (bounds.intersects(vBounds)) {
            vine.disturbance += (velocityX * 0.002f);
        }
    }
}

void Tree::update(float dt) {
    if (harvestState == TreeHarvestState::Falling) {
        fallAngularVelocity += 140.0f * fallDirection * dt;
        fallAngle += fallAngularVelocity * dt;

        if (std::abs(fallAngle) >= 88.0f) {
            fallAngle = (fallDirection > 0.0f) ? 88.0f : -88.0f;
            harvestState = TreeHarvestState::Fading;
            fadeTimer = 0.0f;
        }
    } else if (harvestState == TreeHarvestState::Fading) {
        fadeTimer += dt;
        if (fadeTimer >= maxFadeTime) {
            harvestState = TreeHarvestState::Harvested;
        }
    }
}

void Tree::draw(sf::RenderTarget&) const {}

void Tree::drawCanopy(sf::RenderTarget&, const sf::FloatRect&, ProfilerStats&) const {}

void Tree::drawGeometry(sf::RenderTarget& target, const sf::FloatRect&, ProfilerStats& profiler) const {
    if (harvestState == TreeHarvestState::Harvested) return;

    sf::Transform treeTransform;
    if (harvestState == TreeHarvestState::Falling || harvestState == TreeHarvestState::Fading) {
        sf::Vector2f basePivot(getTrunkCenter(), trunkBounds.top + trunkBounds.height);
        treeTransform.rotate(fallAngle, basePivot);
    }

    sf::RenderStates states;
    states.transform = treeTransform;

    sf::Uint8 alpha = 255;
    if (harvestState == TreeHarvestState::Fading) {
        float factor = 1.0f - std::clamp(fadeTimer / maxFadeTime, 0.0f, 1.0f);
        alpha = static_cast<sf::Uint8>(255 * factor);
    }

    sf::Sprite drawnTrunk = trunkSprite;
    drawnTrunk.setColor(sf::Color(255, 255, 255, alpha));
    float baseY = worldY;

    sf::Transform shadowProj(
        1.f, -globalShadowShearX, globalShadowShearX * baseY,
        0.f, -globalShadowProjY,  (1.f + globalShadowProjY) * baseY,
        0.f, 0.f,                 1.f
    );

    sf::Sprite shadowSpr = trunkSprite;
    sf::Color sColor = globalShadowColor;
    sColor.a = static_cast<sf::Uint8>(sColor.a * (alpha / 255.f));
    shadowSpr.setColor(sColor);

    target.draw(shadowSpr, shadowProj * states.transform);
    target.draw(drawnTrunk, states);
    profiler.drawCalls++;

    if (hasRoot) {
        sf::Sprite drawnRoot = rootSprite;
        drawnRoot.setColor(sf::Color(255, 255, 255, alpha));
        target.draw(drawnRoot, states);
        profiler.drawCalls++;
    }

    if (harvestState == TreeHarvestState::Targeted || harvestState == TreeHarvestState::BeingHarvested) {
        float markerY = trunkBounds.top + 30.f;

        sf::RectangleShape banner(sf::Vector2f(8.f, 22.f));
        banner.setOrigin(4.f, 11.f);
        banner.setPosition(worldX, markerY);
        banner.setFillColor(harvestState == TreeHarvestState::BeingHarvested ? sf::Color(220, 60, 40) : sf::Color(220, 180, 50));
        banner.setOutlineColor(sf::Color(30, 20, 10));
        banner.setOutlineThickness(1.f);
        target.draw(banner, states);

        sf::CircleShape badge(6.f, 4);
        badge.setOrigin(6.f, 6.f);
        badge.setPosition(worldX, markerY - 14.f);
        badge.setFillColor(sf::Color(190, 145, 55));
        badge.setOutlineColor(sf::Color(40, 25, 10));
        badge.setOutlineThickness(1.f);
        target.draw(badge, states);

        if (harvestProgress > 0.0f) {
            float barW = 36.f;
            sf::RectangleShape barBg(sf::Vector2f(barW, 4.f));
            barBg.setOrigin(barW / 2.f, 2.f);
            barBg.setPosition(worldX, markerY + 18.f);
            barBg.setFillColor(sf::Color(40, 30, 20, 200));
            target.draw(barBg, states);

            float fill = std::clamp(harvestProgress / maxHarvestProgress, 0.0f, 1.0f);
            sf::RectangleShape barFill(sf::Vector2f(barW * fill, 4.f));
            barFill.setOrigin(barW / 2.f, 2.f);
            barFill.setPosition(worldX, markerY + 18.f);
            barFill.setFillColor(sf::Color(100, 200, 70));
            target.draw(barFill, states);
        }
    }

    profiler.objectsRendered += 1;
}

sf::FloatRect Tree::getBounds() const { return totalBounds; }

sf::FloatRect Tree::getTrunkBounds() const {
    if (harvestState == TreeHarvestState::Falling || harvestState == TreeHarvestState::Fading || harvestState == TreeHarvestState::Harvested) {
        return sf::FloatRect(0.f, 0.f, 0.f, 0.f);
    }
    return trunkBounds;
}

float Tree::getTrunkCenter() const { return worldX; }

const std::vector<BranchData>& Tree::getBranches() const {
    static const std::vector<BranchData> emptyBranches;
    if (harvestState == TreeHarvestState::Falling || harvestState == TreeHarvestState::Fading || harvestState == TreeHarvestState::Harvested) {
        return emptyBranches;
    }
    return branchData;
}

const std::vector<VineData>& Tree::getVines() const {
    static const std::vector<VineData> emptyVines;
    if (harvestState == TreeHarvestState::Falling || harvestState == TreeHarvestState::Fading || harvestState == TreeHarvestState::Harvested) {
        return emptyVines;
    }
    return vineData;
}