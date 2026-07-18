#include "entities/Tree.h"
#include "world/SeedManager.h"

Tree::Tree(float x, float y, float width, float height, sf::Color trunkColor) {
    trunk.setSize({width, height});
    trunk.setPosition(x, y - height);
    trunk.setFillColor(trunkColor);
}

void Tree::addBranch(float yOffset, float width, bool rightSide, sf::Color color) {
    sf::RectangleShape branch;
    float branchHeight = 16.f; // Standardized thick branches for clear gameplay readability
    branch.setSize({width, branchHeight});
    branch.setFillColor(color);
    branch.setOutlineColor(sf::Color(20, 10, 5)); // Dark outline for visual pop
    branch.setOutlineThickness(1.f);

    float branchX = rightSide ? trunk.getPosition().x + trunk.getSize().x : trunk.getPosition().x - width;
    float branchY = trunk.getPosition().y + trunk.getSize().y - yOffset;
    
    branch.setPosition(branchX, branchY);
    branches.push_back(branch);
}

void Tree::buildCanopy(uint32_t& seed, float baseRadius, float yOffset, sf::Color color, int clusterCount) {
    float centerX = trunk.getPosition().x + (trunk.getSize().x / 2.f);
    float centerY = trunk.getPosition().y + trunk.getSize().y - yOffset;

    for (int i = 0; i < clusterCount; ++i) {
        float r = baseRadius * SeedManager::getRandomFloat(seed, 0.6f, 1.2f);
        float ox = SeedManager::getRandomFloat(seed, -baseRadius * 0.8f, baseRadius * 0.8f);
        float oy = SeedManager::getRandomFloat(seed, -baseRadius * 0.5f, baseRadius * 0.5f);

        sf::CircleShape cluster(r);
        
        sf::Color c = color;
        c.r = static_cast<sf::Uint8>(std::clamp(c.r + SeedManager::getRandomInt(seed, -10, 10), 0, 255));
        c.g = static_cast<sf::Uint8>(std::clamp(c.g + SeedManager::getRandomInt(seed, -15, 15), 0, 255));
        c.b = static_cast<sf::Uint8>(std::clamp(c.b + SeedManager::getRandomInt(seed, -10, 10), 0, 255));
        
        cluster.setFillColor(c);
        cluster.setOrigin(r, r);
        cluster.setPosition(centerX + ox, centerY + oy);
        canopyClusters.push_back(cluster);
    }
}

void Tree::update(float dt) {}

void Tree::draw(sf::RenderWindow& window) const {
    drawCanopy(window);
    drawGeometry(window);
}

void Tree::drawCanopy(sf::RenderWindow& window) const {
    for (const auto& cluster : canopyClusters) window.draw(cluster);
}

void Tree::drawGeometry(sf::RenderWindow& window) const {
    window.draw(trunk);
    for (const auto& branch : branches) window.draw(branch);
}

sf::FloatRect Tree::getBounds() const {
    sf::FloatRect bounds = trunk.getGlobalBounds();
    for (const auto& b : branches) {
        sf::FloatRect bb = b.getGlobalBounds();
        if (bb.left < bounds.left) { bounds.width += (bounds.left - bb.left); bounds.left = bb.left; }
        if (bb.top < bounds.top) { bounds.height += (bounds.top - bb.top); bounds.top = bb.top; }
        if (bb.left + bb.width > bounds.left + bounds.width) bounds.width = (bb.left + bb.width) - bounds.left;
    }
    return bounds;
}

sf::FloatRect Tree::getTrunkBounds() const { return trunk.getGlobalBounds(); }
float Tree::getTrunkCenter() const { return trunk.getPosition().x + (trunk.getSize().x / 2.0f); }
const std::vector<sf::RectangleShape>& Tree::getBranches() const { return branches; }
const std::vector<sf::CircleShape>& Tree::getCanopy() const { return canopyClusters; }