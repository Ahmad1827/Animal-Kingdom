#include "entities/Tree.h"
#include "world/SeedManager.h"
#include <cmath>

Tree::Tree(float x, float y, float width, float height, sf::Color trunkColor) 
    : staticMesh(sf::Triangles), dynamicMesh(sf::Triangles) {
    trunkBounds = sf::FloatRect(x, y - height, width, height);
    totalBounds = trunkBounds;
    appendQuad(staticMesh, trunkBounds, trunkColor);
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

void Tree::addBranch(float yOffset, float width, bool rightSide, sf::Color color) {
    float branchHeight = 16.f; 
    float branchX = rightSide ? trunkBounds.left + trunkBounds.width : trunkBounds.left - width;
    float branchY = trunkBounds.top + trunkBounds.height - yOffset;
    
    sf::FloatRect bBounds(branchX, branchY, width, branchHeight);
    branchData.push_back({bBounds});
    appendQuad(staticMesh, bBounds, color);

    // Expand total bounds
    if (bBounds.left < totalBounds.left) { totalBounds.width += (totalBounds.left - bBounds.left); totalBounds.left = bBounds.left; }
    if (bBounds.top < totalBounds.top) { totalBounds.height += (totalBounds.top - bBounds.top); totalBounds.top = bBounds.top; }
    if (bBounds.left + bBounds.width > totalBounds.left + totalBounds.width) totalBounds.width = (bBounds.left + bBounds.width) - totalBounds.left;
}

void Tree::addVine(float xOffset, float yOffset, float length) {
    sf::Vector2f origin(trunkBounds.left + xOffset, trunkBounds.top + trunkBounds.height - yOffset);
    vineData.push_back({origin, length, 0.f});
    
    // Expand total bounds for vines falling straight down
    if (origin.y + length > totalBounds.top + totalBounds.height) totalBounds.height = (origin.y + length) - totalBounds.top;
}

void Tree::buildCanopy(uint32_t& seed, float baseRadius, float yOffset, sf::Color color, int clusterCount) {
    float centerX = trunkBounds.left + (trunkBounds.width / 2.f);
    float centerY = trunkBounds.top + trunkBounds.height - yOffset;

    for (int i = 0; i < clusterCount; ++i) {
        float r = baseRadius * SeedManager::getRandomFloat(seed, 0.6f, 1.2f);
        float ox = SeedManager::getRandomFloat(seed, -baseRadius * 0.8f, baseRadius * 0.8f);
        float oy = SeedManager::getRandomFloat(seed, -baseRadius * 0.5f, baseRadius * 0.5f);

        sf::Color c = color;
        c.r = static_cast<sf::Uint8>(std::clamp(c.r + SeedManager::getRandomInt(seed, -10, 10), 0, 255));
        c.g = static_cast<sf::Uint8>(std::clamp(c.g + SeedManager::getRandomInt(seed, -15, 15), 0, 255));
        c.b = static_cast<sf::Uint8>(std::clamp(c.b + SeedManager::getRandomInt(seed, -10, 10), 0, 255));
        
        canopyData.push_back({sf::Vector2f(centerX + ox, centerY + oy), r, c});
        
        // Expand total bounds
        if (centerY + oy - r < totalBounds.top) { totalBounds.height += (totalBounds.top - (centerY + oy - r)); totalBounds.top = centerY + oy - r; }
        if (centerX + ox - r < totalBounds.left) { totalBounds.width += (totalBounds.left - (centerX + ox - r)); totalBounds.left = centerX + ox - r; }
        if (centerX + ox + r > totalBounds.left + totalBounds.width) totalBounds.width = (centerX + ox + r) - totalBounds.left;
    }
}

void Tree::initDynamicMesh() {
    // Exactly preallocate to avoid dynamicMesh.clear() during rendering
    // 24 vertices per octagon cluster, 6 vertices per vine quad
    dynamicMesh.resize(canopyData.size() * 24 + vineData.size() * 6);
    
    // Assign base colors once
    size_t vIdx = 0;
    for (const auto& cluster : canopyData) {
        for (int i = 0; i < 24; ++i) dynamicMesh[vIdx++].color = cluster.color;
    }
    for (const auto& vine : vineData) {
        for (int i = 0; i < 6; ++i) dynamicMesh[vIdx++].color = sf::Color(40, 100, 30);
    }
    
    updateSway(0.f, sf::Vector2f(0.f, 0.f)); // Force initial position calculation
}

void Tree::updateSway(float globalTime, const sf::Vector2f& windVector) {
    if (dynamicMesh.getVertexCount() == 0) return;

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
        // Wind + Player Physical Disturbance Decay
        float swayAngle = std::sin(globalTime * 2.f + vine.origin.x * 0.05f) * (0.1f + std::abs(windVector.x) * 0.2f) + (windVector.x * 0.3f);
        swayAngle += vine.disturbance;
        vine.disturbance *= 0.95f; // Damping spring

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

void Tree::update(float dt) {}
void Tree::draw(sf::RenderWindow& window) const {} // Handled via specific layer passes

void Tree::drawCanopy(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    window.draw(dynamicMesh);
    profiler.objectsRendered++;
}

void Tree::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    window.draw(staticMesh);
    profiler.objectsRendered++;
}

sf::FloatRect Tree::getBounds() const { return totalBounds; }
sf::FloatRect Tree::getTrunkBounds() const { return trunkBounds; }
float Tree::getTrunkCenter() const { return trunkBounds.left + (trunkBounds.width / 2.0f); }
const std::vector<BranchData>& Tree::getBranches() const { return branchData; }
const std::vector<VineData>& Tree::getVines() const { return vineData; }