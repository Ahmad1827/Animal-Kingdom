#pragma once
#include <SFML/Graphics.hpp>

class Background {
private:
    sf::Sprite bg1, bg2, bg3;
    float smoothedX = 0.f;
    bool initialized = false;

public:
    Background(class AssetManager& assets);
    void update(float cameraX, float cameraY, sf::Vector2f viewSize, float dt);
    void draw(sf::RenderWindow& window) const;
};