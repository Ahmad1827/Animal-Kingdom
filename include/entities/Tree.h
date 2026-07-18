#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class Tree {
private:
    sf::RectangleShape trunk;
    std::vector<sf::RectangleShape> branches;

public:
    Tree(float x, float y, float width, float height);
    void addBranch(float yOffset, float width, bool rightSide);
    void draw(sf::RenderWindow& window) const;
    sf::FloatRect getTrunkBounds() const;
    float getTrunkCenter() const;
    const std::vector<sf::RectangleShape>& getBranches() const;
};