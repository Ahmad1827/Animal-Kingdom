#pragma once
#include <SFML/Graphics.hpp>
#include "world/WorldObject.h"

class Decoration : public WorldObject {
private:
    sf::ConvexShape shape;
    sf::FloatRect bounds;

public:
    Decoration(float x, float y, int type, float scale, sf::Color color);
    void update(float dt) override;
    void draw(sf::RenderWindow& window) const override;
    sf::FloatRect getBounds() const override;
};