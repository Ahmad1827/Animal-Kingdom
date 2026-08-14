#pragma once
#include <SFML/Graphics.hpp>

class Background {
public:
    Background(class AssetManager& assets);
    void update(float cameraX, float cameraY, sf::Vector2f viewSize, float dt);
    
    void drawSky(sf::RenderTarget& target, sf::Color skyTint);
    void drawDistant(sf::RenderTarget& target, float worldGroundY);
    void drawForeground(sf::RenderTarget& target, float worldGroundY);

private:
    sf::Sprite layer1; 
    sf::Sprite layer2; 
    sf::Sprite layer3; 
    sf::Sprite layer4; 

    float parallax1X, parallax1Y;
    float parallax2X, parallax2Y;
    float parallax3X, parallax3Y;
    float parallax4X, parallax4Y;

    float camX;
    float camY;
    sf::Vector2f vSize;

    void drawLayer(sf::RenderTarget& target, sf::Sprite& spr, float pFactorX, float pFactorY, float targetWorldBottom);
};