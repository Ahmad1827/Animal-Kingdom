#include "world/Decoration.h"
#include "world/SeedManager.h"
#include "core/VisualConfig.h"

Decoration::Decoration(float x, float y, int type, uint32_t seed, sf::Texture& texture) : decorType(type) {
    sprite.setTexture(texture);
    
    sf::IntRect sourceRect = VisualConfig::DECOR_BUSH;
    if (type == 3) sourceRect = VisualConfig::DECOR_ROCK;
    if (type == 4) sourceRect = VisualConfig::DECOR_ROCK;
    
    sprite.setTextureRect(sourceRect);
    sprite.setOrigin(sourceRect.width / 2.f, sourceRect.height);
    sprite.setPosition(x, y);

    float scaleVar = SeedManager::getRandomFloat(seed, 0.8f, 1.2f);
    sprite.setScale(scaleVar, scaleVar);

    collisionBounds = sf::FloatRect(x - (sourceRect.width * scaleVar) / 2.f, y - (sourceRect.height * scaleVar), sourceRect.width * scaleVar, sourceRect.height * scaleVar);
}

void Decoration::update(float dt) {}
void Decoration::draw(sf::RenderTarget& target) const { 
    target.draw(sprite); 
}
sf::FloatRect Decoration::getBounds() const { return collisionBounds; }
sf::Vector2f Decoration::getOrigin() const { return sprite.getOrigin(); }