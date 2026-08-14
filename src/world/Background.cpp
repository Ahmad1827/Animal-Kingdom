#include "world/Background.h"
#include "core/AssetManager.h"
#include <cmath>

Background::Background(AssetManager& assets) {
    sf::Texture& t1 = assets.getTexture("layer1_sky");
    sf::Texture& t2 = assets.getTexture("layer2_mountains");
    sf::Texture& t3 = assets.getTexture("layer3_hills");
    sf::Texture& t4 = assets.getTexture("layer4_foreground");

    t1.setRepeated(false);
    t2.setRepeated(false);
    t3.setRepeated(false);
    t4.setRepeated(false);

    t1.setSmooth(false);
    t2.setSmooth(false);
    t3.setSmooth(false);
    t4.setSmooth(false);

    layer1.setTexture(t1);
    layer2.setTexture(t2);
    layer3.setTexture(t3);
    layer4.setTexture(t4);

    parallax1X = 0.00f;
    parallax1Y = 0.00f;
    parallax2X = 0.12f;
    parallax2Y = 0.10f;
    parallax3X = 0.35f;
    parallax3Y = 0.25f;
    parallax4X = 1.15f;
    parallax4Y = 1.00f;
}

void Background::update(float cameraX, float cameraY, sf::Vector2f viewSize, float dt) {
    camX = cameraX;
    camY = cameraY;
    vSize = viewSize;
}

void Background::drawLayer(sf::RenderTarget& target, sf::Sprite& spr, float pFactorX, float pFactorY, float targetWorldBottom) {
    const sf::Texture* tex = spr.getTexture();
    if (!tex) return;

    float baseHeight = static_cast<float>(layer1.getTexture()->getSize().y);
    float globalScale = vSize.y / baseHeight;
    
    spr.setScale(globalScale, globalScale);

    float texW = static_cast<float>(tex->getSize().x) * globalScale;
    float texH = static_cast<float>(tex->getSize().y) * globalScale;

    float overlapTexW = std::floor(texW) - 1.f;

    float rawOffsetX = camX * pFactorX;
    float modX = std::fmod(rawOffsetX, overlapTexW);
    if (modX < 0.f) modX += overlapTexW;

    float startX = std::floor(camX - (vSize.x / 2.f) - modX);
    
    float targetWorldTop = targetWorldBottom - texH;
    float drawY = targetWorldTop + (camY - targetWorldTop) * (1.0f - pFactorY);

    int tilesNeeded = static_cast<int>(std::ceil(vSize.x / overlapTexW)) + 2;

    for (int i = 0; i < tilesNeeded; ++i) {
        spr.setPosition(std::floor(startX + (i * overlapTexW)), std::floor(drawY));
        target.draw(spr);
    }
}

void Background::drawSky(sf::RenderTarget& target, sf::Color skyTint) {
    layer1.setColor(skyTint);
    drawLayer(target, layer1, parallax1X, parallax1Y, camY + vSize.y / 2.f);
}

void Background::drawDistant(sf::RenderTarget& target, float worldGroundY) {
    layer2.setColor(sf::Color::White);
    layer3.setColor(sf::Color::White);

    float bottom2 = worldGroundY + (vSize.y * 0.30f); 
    float bottom3 = worldGroundY + (vSize.y * 0.20f);

    drawLayer(target, layer2, parallax2X, parallax2Y, bottom2);
    drawLayer(target, layer3, parallax3X, parallax3Y, bottom3);
}

void Background::drawForeground(sf::RenderTarget& target, float worldGroundY) {
    layer4.setColor(sf::Color::White);
    float bottom4 = worldGroundY + (vSize.y * 0.25f);
    drawLayer(target, layer4, parallax4X, parallax4Y, bottom4);
}