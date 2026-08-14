#include "world/Background.h"
#include "core/AssetManager.h"
#include <cmath>
#include <algorithm>

Background::Background(AssetManager& assets) {
    sf::Texture& t1 = assets.getTexture("layer1_sky");
    sf::Texture& t2 = assets.getTexture("layer2_mountains");
    sf::Texture& t3 = assets.getTexture("layer3_hills");
    sf::Texture& t4 = assets.getTexture("layer4_foreground");

    t1.setRepeated(false); t2.setRepeated(false);
    t3.setRepeated(false); t4.setRepeated(false);

    t1.setSmooth(false); t2.setSmooth(false);
    t3.setSmooth(false); t4.setSmooth(false);

    layer1_sky.sprite.setTexture(t1);
    layer2_mountains.sprite.setTexture(t2);
    layer3_hills.sprite.setTexture(t3);
    layer4_foreground.sprite.setTexture(t4);

    layer1_sky.parallaxX = 0.00f;
    layer2_mountains.parallaxX = 0.10f;
    layer3_hills.parallaxX = 0.30f;
    layer4_foreground.parallaxX = 0.60f;

    layer1_sky.scale = 1.0f;
    layer2_mountains.scale = 0.35f;
    layer3_hills.scale = 0.32f;
    layer4_foreground.scale = 0.16f;

    // FIX 1: Push the distant layers down so they dip behind the 
    // physical dirt line, completely hiding the blue sky gap.
    layer1_sky.yOffset = 0.f;
    layer2_mountains.yOffset = 25.f; 
    layer3_hills.yOffset = 25.f;
    layer4_foreground.yOffset = -10.f;

    findVisibleBottom(layer1_sky);
    findVisibleBottom(layer2_mountains);
    findVisibleBottom(layer3_hills);
    findVisibleBottom(layer4_foreground);

    camX = 0.f; camY = 0.f;
    vSize = sf::Vector2f(1280.f, 720.f);
}

void Background::findVisibleBottom(Layer& layer) {
    const sf::Texture* tex = layer.sprite.getTexture();
    if (!tex) { layer.visibleBottomY = 0; return; }

    sf::Image img = tex->copyToImage();
    unsigned int width = img.getSize().x;
    unsigned int height = img.getSize().y;
    int lastY = -1;

    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            if (img.getPixel(x, y).a > 15) {
                lastY = static_cast<int>(y);
                break;
            }
        }
    }
    layer.visibleBottomY = (lastY != -1) ? lastY : static_cast<int>(height);
}

void Background::update(float cameraX, float cameraY, sf::Vector2f viewSize, float dt) {
    camX = cameraX; camY = cameraY; vSize = viewSize;
}

void Background::renderTiledLayer(sf::RenderTarget& target, Layer& layer, float targetWorldGroundY, bool isSky) {
    const sf::Texture* tex = layer.sprite.getTexture();
    if (!tex) return;

    float currentScale = layer.scale;
    if (isSky) currentScale = vSize.y / static_cast<float>(tex->getSize().y);

    layer.sprite.setScale(currentScale, currentScale);

    float scaledTexW = static_cast<float>(tex->getSize().x) * currentScale;
    float overlapTexW = std::max(1.0f, std::floor(scaledTexW) - 1.0f);

    float rawOffset = camX * layer.parallaxX;
    float modX = std::fmod(rawOffset, overlapTexW);
    if (modX < 0.f) modX += overlapTexW;

    float startX = std::floor(camX - (vSize.x / 2.f) - modX);
    float drawY = 0.f;

    if (isSky) {
        drawY = std::floor(camY - (vSize.y / 2.f));
    } else {
        float visibleBottomOffset = static_cast<float>(layer.visibleBottomY) * currentScale;
        drawY = std::floor(targetWorldGroundY - visibleBottomOffset + layer.yOffset);
    }

    int tilesNeeded = static_cast<int>(std::ceil(vSize.x / overlapTexW)) + 2;

    for (int i = 0; i < tilesNeeded; ++i) {
        layer.sprite.setPosition(std::floor(startX + (i * overlapTexW)), drawY);
        target.draw(layer.sprite);
    }
}

void Background::drawSky(sf::RenderTarget& target, sf::Color skyTint) {
    layer1_sky.sprite.setColor(skyTint);
    renderTiledLayer(target, layer1_sky, GROUND_BASELINE_Y, true);
}

void Background::drawDistant(sf::RenderTarget& target, float worldGroundY) {
    layer2_mountains.sprite.setColor(sf::Color::White);
    layer3_hills.sprite.setColor(sf::Color::White);

    renderTiledLayer(target, layer2_mountains, worldGroundY, false);
    renderTiledLayer(target, layer3_hills, worldGroundY, false);
}

void Background::drawForeground(sf::RenderTarget& target, float worldGroundY) {
    // Hidden temporarily
}