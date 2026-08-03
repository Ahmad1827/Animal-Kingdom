#pragma once
#include "world/WorldObject.h"
#include <SFML/Graphics.hpp>
#include <cstdint>

class Decoration : public WorldObject {
private:
    sf::Sprite sprite;
    sf::FloatRect collisionBounds;
    int decorType;

public:
    Decoration(float x, float y, int type, uint32_t seed, sf::Texture& texture);
    
    void update(float dt) override;
    void draw(sf::RenderTarget& target) const;
    void draw(sf::RenderWindow& window) const override { draw(static_cast<sf::RenderTarget&>(window)); }
    sf::FloatRect getBounds() const override;
    sf::Vector2f getOrigin() const;
};