#pragma once
#include <SFML/Graphics.hpp>

class WorldObject {
public:
    virtual ~WorldObject() = default;
    virtual void update(float dt) = 0;
    virtual void draw(sf::RenderWindow& window) const = 0;
    virtual sf::FloatRect getBounds() const = 0;
};