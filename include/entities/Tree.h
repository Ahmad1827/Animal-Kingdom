#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include "world/WorldObject.h"

class Tree : public WorldObject {
private:
    sf::RectangleShape trunk;
    std::vector<sf::RectangleShape> branches;
    std::vector<sf::CircleShape> canopyClusters;

public:
    Tree(float x, float y, float width, float height, sf::Color trunkColor);
    void addBranch(float yOffset, float width, bool rightSide, sf::Color color);
    void buildCanopy(uint32_t& seed, float baseRadius, float yOffset, sf::Color color, int clusterCount);
   void update(float dt) override;
    void draw(sf::RenderWindow& window) const override;
    void drawCanopy(sf::RenderWindow& window) const;
    void drawGeometry(sf::RenderWindow& window) const;
    sf::FloatRect getBounds() const override;
    sf::FloatRect getTrunkBounds() const;
    float getTrunkCenter() const;
    const std::vector<sf::RectangleShape>& getBranches() const;
    const std::vector<sf::CircleShape>& getCanopy() const;
};