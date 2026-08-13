#include "world/Background.h"
#include "core/AssetManager.h"
#include <cmath>

Background::Background(AssetManager& assets) {
    sf::Texture& t1 = assets.getTexture("LAYER1FIX");
    sf::Texture& t2 = assets.getTexture("LAYER2FIX");
    sf::Texture& t3 = assets.getTexture("LAYER3FIX");
    sf::Texture& t4 = assets.getTexture("LAYER4FIX");

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

    parallax1 = 0.00f;
    parallax2 = 0.10f;
    parallax3 = 0.25f;
    parallax4 = 0.45f;
}

void Background::update(float cameraX, float cameraY, sf::Vector2f viewSize, float dt) {
    camX = cameraX;
    camY = cameraY;
    vSize = viewSize;
}

void Background::drawLayer(sf::RenderTarget& target, sf::Sprite& spr, float pFactor, bool anchorBottom, float yOffset) {
    const sf::Texture* tex = spr.getTexture();
    if (!tex) return;

    float baseHeight = static_cast<float>(layer1.getTexture()->getSize().y);
    float globalScale = vSize.y / baseHeight;
    
    spr.setScale(globalScale, globalScale);

    float texW = static_cast<float>(tex->getSize().x) * globalScale;
    float texH = static_cast<float>(tex->getSize().y) * globalScale;

    float overlapTexW = std::floor(texW) - 1.f;

    float rawOffset = camX * pFactor;
    float modX = std::fmod(rawOffset, overlapTexW);
    if (modX < 0.f) modX += overlapTexW;

    float startX = std::floor(camX - (vSize.x / 2.f) - modX);
    
    float drawY;
    if (anchorBottom) {
        drawY = std::floor(camY + (vSize.y / 2.f) - texH + yOffset);
    } else {
        drawY = std::floor(camY - (vSize.y / 2.f));
    }

    int tilesNeeded = static_cast<int>(std::ceil(vSize.x / overlapTexW)) + 2;

    for (int i = 0; i < tilesNeeded; ++i) {
        spr.setPosition(std::floor(startX + (i * overlapTexW)), drawY);
        target.draw(spr);
    }
}

void Background::drawSky(sf::RenderTarget& target, sf::Color skyTint) {
    layer1.setColor(skyTint);
    drawLayer(target, layer1, parallax1, false, 0.f);
}

void Background::drawParallax(sf::RenderTarget& target) {
    layer2.setColor(sf::Color::White);
    layer3.setColor(sf::Color::White);
    layer4.setColor(sf::Color::White);

    float groundOffset = vSize.y * 0.15f; 

    drawLayer(target, layer2, parallax2, true, groundOffset);
    drawLayer(target, layer3, parallax3, true, groundOffset);
    drawLayer(target, layer4, parallax4, true, groundOffset);
}