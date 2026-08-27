#include "world/Decoration.h"
#include "world/SeedManager.h"
#include "core/VisualConfig.h"

Decoration::Decoration(float x, float y, int type, uint32_t seed, sf::Texture& texture) : decorType(type) {
    sprite.setTexture(texture);
    
    sf::IntRect sourceRect = VisualConfig::DECOR_BUSH;
    
    if (type == 2) {
        int v = seed % 4;
        if (v == 0) sourceRect = VisualConfig::DECOR_FERN;
        else if (v == 1) sourceRect = VisualConfig::DECOR_BUSH_SMALL;
        else if (v == 2) sourceRect = VisualConfig::DECOR_ROOT;
        else sourceRect = VisualConfig::DECOR_BUSH;
    } else if (type == 3) {
        sourceRect = (seed % 2 == 0) ? VisualConfig::DECOR_ROCK : VisualConfig::DECOR_ROCK_SMALL;
    } else if (type == 4) {
        sourceRect = VisualConfig::DECOR_ROCK_SMALL;
    } else if (type == 1) {
        sourceRect = VisualConfig::DECOR_BUSH_SMALL;
    }
    
    sprite.setTextureRect(sourceRect);
    sprite.setOrigin(sourceRect.width / 2.f, static_cast<float>(sourceRect.height));
    sprite.setPosition(x, y);

    float scaleVar = SeedManager::getRandomFloat(seed, 0.85f, 1.15f);
    sprite.setScale(scaleVar, scaleVar);

    collisionBounds = sf::FloatRect(x - (sourceRect.width * scaleVar) / 2.f, y - (sourceRect.height * scaleVar), sourceRect.width * scaleVar, sourceRect.height * scaleVar);
}

void Decoration::update(float) {}

void Decoration::draw(sf::RenderTarget& target) const { 
    target.draw(sprite); 
}

sf::FloatRect Decoration::getBounds() const { return collisionBounds; }

sf::Vector2f Decoration::getOrigin() const { return sprite.getOrigin(); }