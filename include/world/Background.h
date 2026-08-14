#pragma once
#include <SFML/Graphics.hpp>

class Background {
public:
    Background(class AssetManager& assets);
    void update(float cameraX, float cameraY, sf::Vector2f viewSize, float dt);
    
    void drawSky(sf::RenderTarget& target, sf::Color skyTint);
    void drawDistant(sf::RenderTarget& target);
    void drawForeground(sf::RenderTarget& target);

private:
    sf::Sprite layer1; 
    sf::Sprite layer2; 
    sf::Sprite layer3; 
    sf::Sprite layer4; 

    float parallax1;
    float parallax2;
    float parallax3;
    float parallax4;

    float camX;
    float camY;
    sf::Vector2f vSize;

    void drawLayer(sf::RenderTarget& target, sf::Sprite& spr, float pFactor, bool anchorBottom, float yOffset);
};