#include "world/Background.h"
#include "core/AssetManager.h"
#include "core/VisualConfig.h"
#include <cmath>

Background::Background(AssetManager& assets) {
    sf::Texture& t1 = assets.getTexture("bg1");
    sf::Texture& t2 = assets.getTexture("bg2");
    sf::Texture& t3 = assets.getTexture("bg3");
    t1.setRepeated(true);
    t2.setRepeated(true);
    t3.setRepeated(true);
    bg1.setTexture(t1);
    bg2.setTexture(t2);
    bg3.setTexture(t3);
}

void Background::update(float cameraX, float cameraY, sf::Vector2f viewSize, float dt) {
    const float texW = 320.f;
    const float texH = 180.f;

    float scale = (viewSize.y / texH) * 1.15f;
    float scaledTexH = texH * scale;
    int tilesNeeded = static_cast<int>(std::ceil(viewSize.x / (texW * scale))) + 2;
    float drawWSrc = texW * tilesNeeded;

    auto applyLayer = [&](sf::Sprite& spr, float speed) {
        spr.setScale(scale, scale);
        
        // FIX: Directly use the cameraX. No secondary interpolation!
        float rawOffset = cameraX * speed; 
        float wrapped = std::fmod(rawOffset, texW);
        if (wrapped < 0.f) wrapped += texW;

        spr.setTextureRect(sf::IntRect(static_cast<int>(wrapped), 0,
                                       static_cast<int>(drawWSrc), static_cast<int>(texH)));
                                       
        // FIX: Floor the coordinates to prevent subpixel rendering artifacts
        spr.setPosition(std::floor(cameraX - (drawWSrc * scale) / 2.f), std::floor(cameraY - scaledTexH / 2.f)); 
    };

    applyLayer(bg1, VisualConfig::PARALLAX_FAR_SPEED);
    applyLayer(bg2, VisualConfig::PARALLAX_MID_SPEED);
    applyLayer(bg3, VisualConfig::PARALLAX_NEAR_SPEED);
}

void Background::draw(sf::RenderWindow& window) const {
    window.draw(bg1);
    window.draw(bg2);
    window.draw(bg3);
}