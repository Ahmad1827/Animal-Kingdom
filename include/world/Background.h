#pragma once
#include <SFML/Graphics.hpp>

class Background {
public:
    static constexpr float GROUND_BASELINE_Y = 500.0f;

    Background(class AssetManager& assets);
    void update(float cameraX, float cameraY, sf::Vector2f viewSize, float dt);
    
    void drawSky(sf::RenderTarget& target, sf::Color skyTint);
    void drawDistant(sf::RenderTarget& target, float worldGroundY);
    void drawForeground(sf::RenderTarget& target, float worldGroundY);

private:
    struct Layer {
        sf::Sprite sprite;
        float parallaxX;
        float scale;
        float yOffset;
        int visibleBottomY;
    };

    Layer layer1_sky;
    Layer layer2_mountains;
    Layer layer3_hills;
    Layer layer4_foreground;

    float camX;
    float camY;
    sf::Vector2f vSize;

    void findVisibleBottom(Layer& layer);
    void renderTiledLayer(sf::RenderTarget& target, Layer& layer, float targetWorldGroundY, bool isSky);
};