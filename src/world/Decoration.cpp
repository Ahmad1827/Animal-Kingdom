#include "world/Decoration.h"
#include "world/SeedManager.h"

Decoration::Decoration(float x, float y, int type, uint32_t seed, sf::Texture& texture) : decorType(type) {
    sprite.setTexture(texture);
    
    float width = 32.f;
    float height = 32.f;
    
    if (type == 0) { width = 20.f; height = 15.f; }
    else if (type == 1) { width = 10.f; height = 25.f; }
    else if (type == 2) { width = 40.f; height = 30.f; }
    else if (type == 3) { width = 30.f; height = 15.f; }
    else if (type == 4) { width = 80.f; height = 18.f; }

    sprite.setTextureRect(sf::IntRect(0, 0, static_cast<int>(width), static_cast<int>(height)));
    
    sprite.setOrigin(width / 2.f, height);
    sprite.setPosition(x, y);

    if (type == 0) sprite.setColor(sf::Color(50, 180, 50));
    else if (type == 1) sprite.setColor(sf::Color(200, 50, 50));
    else if (type == 2) sprite.setColor(sf::Color(20, 100, 30));
    else if (type == 3) sprite.setColor(sf::Color(100, 100, 100));
    else if (type == 4) sprite.setColor(sf::Color(80, 50, 30));

    collisionBounds = sf::FloatRect(x - width / 2.f, y - height, width, height);
}

void Decoration::update(float dt) {}
void Decoration::draw(sf::RenderWindow& window) const { window.draw(sprite); }
sf::FloatRect Decoration::getBounds() const { return collisionBounds; }
sf::Vector2f Decoration::getOrigin() const { return sprite.getOrigin(); }