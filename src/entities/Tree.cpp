#include "entities/Tree.h"
#include "world/SeedManager.h"
#include <cmath>
#include "core/VisualConfig.h"
#include <algorithm>

Tree::Tree(float x, float y, float width, float height, sf::Color trunkColor, sf::Texture& decorTexture, int id) 
    : treeId(id) {
    trunkBounds = sf::FloatRect(x - width / 2.f, y - height, width, height);
    totalBounds = trunkBounds;

    trunkSprite.setTexture(decorTexture);
    trunkSprite.setTextureRect(VisualConfig::DECOR_TREE);
    trunkSprite.setOrigin(VisualConfig::DECOR_TREE.width / 2.f, VisualConfig::DECOR_TREE.height);
    trunkSprite.setPosition(x, y);
    
    float scale = height / VisualConfig::DECOR_TREE.height;
    trunkSprite.setScale(scale, scale); 
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

void Tree::addBranch(float yOffset, float width, bool rightSide, sf::Color color, sf::Texture& decorTexture) {
    float startX = trunkBounds.left + (rightSide ? trunkBounds.width : 0.f);
    float bHeight = 45.f; 

    sf::FloatRect branchRect(rightSide ? startX : startX - width, trunkBounds.top + trunkBounds.height - yOffset, width, bHeight);
    branchData.push_back({branchRect}); 

    float bushW = static_cast<float>(VisualConfig::DECOR_BUSH.width);
    float bushH = static_cast<float>(VisualConfig::DECOR_BUSH.height);
    float uniformScale = bHeight / bushH;
    float drawnW = bushW * uniformScale;

    int count = std::max(1, static_cast<int>(std::ceil(width / (drawnW * 0.7f)))); 
    float step = (count > 1) ? (width - drawnW) / (count - 1) : 0.f;

    branchSprites.reserve(branchSprites.size() + count);

    for (int i = 0; i < count; ++i) {
        float localX = rightSide ? (startX + i * step) : (startX - width + i * step);

        sf::Sprite bSprite(decorTexture);
        bSprite.setTextureRect(VisualConfig::DECOR_BUSH);
        bSprite.setOrigin(bushW / 2.f, bushH / 2.f);
        bSprite.setPosition(localX + drawnW / 2.f, trunkBounds.top + trunkBounds.height - yOffset + bHeight / 2.f);
        bSprite.setScale(uniformScale, uniformScale);

        branchSprites.push_back(bSprite);
    }
}

void Tree::addVine(float xOffset, float yOffset, float length) {
    sf::Vector2f origin(trunkBounds.left + xOffset, trunkBounds.top + trunkBounds.height - yOffset);
    vineData.push_back({origin, length, 0.f});
    
    if (origin.y + length > totalBounds.top + totalBounds.height) totalBounds.height = (origin.y + length) - totalBounds.top;
}

void Tree::buildCanopy(uint32_t& seed, float baseRadius, float yOffset, sf::Color color, int clusterCount) {
    float centerX = trunkBounds.left + (trunkBounds.width / 2.f);
    float centerY = trunkBounds.top + trunkBounds.height - yOffset;

    canopyData.reserve(canopyData.size() + clusterCount);

    for (int i = 0; i < clusterCount; ++i) {
        float r = baseRadius * SeedManager::getRandomFloat(seed, 0.6f, 1.2f);
        float ox = SeedManager::getRandomFloat(seed, -baseRadius * 0.8f, baseRadius * 0.8f);
        float oy = SeedManager::getRandomFloat(seed, -baseRadius * 0.5f, baseRadius * 0.5f);

        sf::Color c = color;
        c.r = static_cast<sf::Uint8>(std::clamp(c.r + SeedManager::getRandomInt(seed, -10, 10), 0, 255));
        c.g = static_cast<sf::Uint8>(std::clamp(c.g + SeedManager::getRandomInt(seed, -15, 15), 0, 255));
        c.b = static_cast<sf::Uint8>(std::clamp(c.b + SeedManager::getRandomInt(seed, -10, 10), 0, 255));
        
        canopyData.push_back({sf::Vector2f(centerX + ox, centerY + oy), r, c});
        
        if (centerY + oy - r < totalBounds.top) { totalBounds.height += (totalBounds.top - (centerY + oy - r)); totalBounds.top = centerY + oy - r; }
        if (centerX + ox - r < totalBounds.left) { totalBounds.width += (totalBounds.left - (centerX + ox - r)); totalBounds.left = centerX + ox - r; }
        if (centerX + ox + r > totalBounds.left + totalBounds.width) totalBounds.width = (centerX + ox + r) - totalBounds.left;
    }
}

void Tree::initDynamicMesh() {
    dynamicMesh.resize(canopyData.size() * 24 + vineData.size() * 6);
    
    size_t vIdx = 0;
    for (const auto& cluster : canopyData) {
        for (int i = 0; i < 24; ++i) dynamicMesh[vIdx++].color = cluster.color;
    }
    for (const auto& vine : vineData) {
        for (int i = 0; i < 6; ++i) dynamicMesh[vIdx++].color = sf::Color(40, 100, 30);
    }
    
    updateSway(0.f, sf::Vector2f(0.f, 0.f)); 
}

void Tree::updateSway(float globalTime, const sf::Vector2f& windVector) {
    if (dynamicMesh.getVertexCount() == 0 || harvestState == TreeHarvestState::Falling) return;

    size_t vIdx = 0;
    
    for (const auto& cluster : canopyData) {
        float sway = std::sin(globalTime * 1.5f + cluster.center.x * 0.01f) * (2.f + std::abs(windVector.x) * 5.f) + (windVector.x * 10.f);
        const int segments = 8;
        for (int i = 0; i < segments; ++i) {
            float a1 = (i * 3.14159f * 2.f) / segments;
            float a2 = ((i + 1) * 3.14159f * 2.f) / segments;
            dynamicMesh[vIdx++].position = cluster.center + sf::Vector2f(sway, 0.f);
            dynamicMesh[vIdx++].position = cluster.center + sf::Vector2f(sway + std::cos(a1) * cluster.radius, std::sin(a1) * cluster.radius);
            dynamicMesh[vIdx++].position = cluster.center + sf::Vector2f(sway + std::cos(a2) * cluster.radius, std::sin(a2) * cluster.radius);
        }
    }
    
    for (auto& vine : vineData) {
        float swayAngle = std::sin(globalTime * 2.f + vine.origin.x * 0.05f) * (0.1f + std::abs(windVector.x) * 0.2f) + (windVector.x * 0.3f);
        swayAngle += vine.disturbance;
        vine.disturbance *= 0.95f; 

        sf::Vector2f endPoint(vine.origin.x + std::sin(swayAngle) * vine.length, vine.origin.y + std::cos(swayAngle) * vine.length);
        sf::Vector2f perp(std::cos(swayAngle) * 3.f, -std::sin(swayAngle) * 3.f);
        
        dynamicMesh[vIdx++].position = vine.origin - perp;
        dynamicMesh[vIdx++].position = vine.origin + perp;
        dynamicMesh[vIdx++].position = endPoint - perp;
        dynamicMesh[vIdx++].position = endPoint - perp;
        dynamicMesh[vIdx++].position = vine.origin + perp;
        dynamicMesh[vIdx++].position = endPoint + perp;
    }
}

void Tree::disturbVines(const sf::FloatRect& bounds, float velocityX) {
    for (auto& vine : vineData) {
        sf::FloatRect vBounds(vine.origin.x - 3.f, vine.origin.y, 6.f, vine.length);
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
            harvestState = TreeHarvestState::Harvested;
        }
    }
}

void Tree::draw(sf::RenderTarget& target) const {} 

void Tree::drawCanopy(sf::RenderTarget& target, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    if (harvestState == TreeHarvestState::Harvested) return;
    target.draw(dynamicMesh);
    profiler.objectsRendered++;
    profiler.drawCalls++;
}

void Tree::drawGeometry(sf::RenderTarget& target, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    if (harvestState == TreeHarvestState::Harvested) return;

    sf::Transform treeTransform;
    if (harvestState == TreeHarvestState::Falling) {
        sf::Vector2f basePivot(getTrunkCenter(), trunkBounds.top + trunkBounds.height);
        treeTransform.rotate(fallAngle, basePivot);
    }

    sf::RenderStates states;
    states.transform = treeTransform;

    target.draw(trunkSprite, states);
    profiler.drawCalls++;
    
    for (const auto& bs : branchSprites) {
        target.draw(bs, states);
        profiler.drawCalls++;
    }
    if (dynamicMesh.getVertexCount() > 0) {
        target.draw(dynamicMesh, states);
        profiler.drawCalls++;
    }

    if (harvestState == TreeHarvestState::Targeted || harvestState == TreeHarvestState::BeingHarvested) {
        float centerX = getTrunkCenter();
        float markerY = trunkBounds.top + 40.f;

        sf::RectangleShape banner(sf::Vector2f(8.f, 22.f));
        banner.setOrigin(4.f, 11.f);
        banner.setPosition(centerX, markerY);
        banner.setFillColor(harvestState == TreeHarvestState::BeingHarvested ? sf::Color(220, 60, 40) : sf::Color(220, 180, 50));
        banner.setOutlineColor(sf::Color(30, 20, 10));
        banner.setOutlineThickness(1.f);
        target.draw(banner);

        sf::CircleShape badge(6.f, 4);
        badge.setOrigin(6.f, 6.f);
        badge.setPosition(centerX, markerY - 14.f);
        badge.setFillColor(sf::Color(190, 145, 55));
        badge.setOutlineColor(sf::Color(40, 25, 10));
        badge.setOutlineThickness(1.f);
        target.draw(badge);

        if (harvestProgress > 0.0f) {
            float barW = 36.f;
            sf::RectangleShape barBg(sf::Vector2f(barW, 4.f));
            barBg.setOrigin(barW / 2.f, 2.f);
            barBg.setPosition(centerX, markerY + 18.f);
            barBg.setFillColor(sf::Color(40, 30, 20, 200));
            target.draw(barBg);

            float fill = std::clamp(harvestProgress / maxHarvestProgress, 0.0f, 1.0f);
            sf::RectangleShape barFill(sf::Vector2f(barW * fill, 4.f));
            barFill.setOrigin(barW / 2.f, 2.f);
            barFill.setPosition(centerX, markerY + 18.f);
            barFill.setFillColor(sf::Color(100, 200, 70));
            target.draw(barFill);
        }
    }
    
    profiler.objectsRendered += 1 + branchSprites.size() + (dynamicMesh.getVertexCount() > 0 ? 1 : 0);
}

sf::FloatRect Tree::getBounds() const { return totalBounds; }
sf::FloatRect Tree::getTrunkBounds() const { return trunkBounds; }
float Tree::getTrunkCenter() const { return trunkBounds.left + (trunkBounds.width / 2.0f); }
const std::vector<BranchData>& Tree::getBranches() const { return branchData; }
const std::vector<VineData>& Tree::getVines() const { return vineData; }