#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

struct Layer {
    sf::RectangleShape shape1;
    sf::RectangleShape shape2;
    float speedMultiplier;
    float textureWidth;
};

class ParallaxBackground {
private:
    std::vector<Layer> layers;

public:
    ParallaxBackground();
    void update(float cameraX);
    void draw(sf::RenderWindow& window) const;
};