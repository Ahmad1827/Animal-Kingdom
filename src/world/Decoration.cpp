#include "world/Decoration.h"
#include "world/SeedManager.h"
#include "core/VisualConfig.h"

Decoration::Decoration(float x, float y, int type, uint32_t seed, sf::Texture& texture) : decorType(type) {
    sprite.setTexture(texture);
    
    // 0=Grass, 1=Flower, 2=Fern map to BUSH_RECT in this basic pack implementation
    // 3=Rock maps to ROCK_RECT
    // 4=Log maps to a rotated or scaled trunk slice
    
    sf::IntRect sourceRect = VisualConfig::DECOR_BUSH;
    if (type == 3) sourceRect = VisualConfig::DECOR_ROCK;
    if (type == 4) sourceRect = VisualConfig::DECOR_ROCK; // Default fallback to rock if log isn't in pack
    
    sprite.setTextureRect(sourceRect);
    sprite.setOrigin(sourceRect.width / 2.f, sourceRect.height);
    sprite.setPosition(x, y);

    // Add slight random scaling based on seed for variety
    float scaleVar = SeedManager::getRandomFloat(seed, 0.8f, 1.2f);
    sprite.setScale(scaleVar, scaleVar);

    collisionBounds = sf::FloatRect(x - (sourceRect.width * scaleVar) / 2.f, y - (sourceRect.height * scaleVar), sourceRect.width * scaleVar, sourceRect.height * scaleVar);
}

void Decoration::update(float dt) {}
void Decoration::draw(sf::RenderWindow& window) const { 
    window.draw(sprite); 
}
sf::FloatRect Decoration::getBounds() const { return collisionBounds; }
sf::Vector2f Decoration::getOrigin() const { return sprite.getOrigin(); }