#include "entities/Tree.h"

Tree::Tree(float x, float y, float width, float height) {
    trunk.setSize({width, height});
    trunk.setPosition(x, y - height);
    trunk.setFillColor(sf::Color(75, 45, 15));
}

void Tree::addBranch(float yOffset, float width, bool rightSide) {
    sf::RectangleShape branch;
    float branchHeight = 15.f;
    branch.setSize({width, branchHeight});
    branch.setFillColor(sf::Color(90, 60, 20));

    float branchX = rightSide ? trunk.getPosition().x + trunk.getSize().x : trunk.getPosition().x - width;
    float branchY = trunk.getPosition().y + trunk.getSize().y - yOffset;
    
    branch.setPosition(branchX, branchY);
    branches.push_back(branch);
}

void Tree::draw(sf::RenderWindow& window) const {
    window.draw(trunk);
    for (const auto& branch : branches) {
        window.draw(branch);
    }
}

sf::FloatRect Tree::getTrunkBounds() const {
    return trunk.getGlobalBounds();
}

float Tree::getTrunkCenter() const {
    return trunk.getPosition().x + (trunk.getSize().x / 2.0f);
}

const std::vector<sf::RectangleShape>& Tree::getBranches() const {
    return branches;
}