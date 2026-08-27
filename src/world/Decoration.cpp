#include "world/Decoration.h"
#include "world/SeedManager.h"
#include "core/VisualConfig.h"

// Decoration types (see DecorType in WorldGenerator.h):
//   0 generic bush | 1 small bush | 2 jungle undergrowth | 3 rock
//   4 small rock   | 5 flower     | 6 leaf litter        | 7 grass tuft
//
// NOTE on 5 / 6 / 7: Decors.png has no dedicated flower or litter art, so
// these reuse existing atlas rects at a smaller scale with a colour tint.
// When you add real flower/litter cells to the atlas, add rects to
// VisualConfig and swap the `sourceRect` lines below - nothing else changes.
Decoration::Decoration(float x, float y, int type, uint32_t seed, sf::Texture& texture) : decorType(type) {
    sprite.setTexture(texture);

    sf::IntRect sourceRect = VisualConfig::DECOR_BUSH;
    sf::Color tint = sf::Color::White;
    float scaleMin = 0.85f;
    float scaleMax = 1.15f;

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
    } else if (type == 5) {
        // Flower: small silhouette, bright petal tint. Field biome signature.
        sourceRect = (seed % 2 == 0) ? VisualConfig::DECOR_BUSH_SMALL : VisualConfig::DECOR_FERN;
        scaleMin = 0.45f;
        scaleMax = 0.72f;
        switch ((seed >> 5) % 5) {
            case 0: tint = sf::Color(255, 225, 120); break; // yellow
            case 1: tint = sf::Color(240, 150, 200); break; // pink
            case 2: tint = sf::Color(235, 235, 245); break; // white
            case 3: tint = sf::Color(180, 160, 245); break; // violet
            default: tint = sf::Color(250, 170, 110); break; // orange
        }
    } else if (type == 6) {
        // Leaf litter: flat, dark, low-contrast. Adds ground depth without noise.
        sourceRect = VisualConfig::DECOR_ROOT;
        scaleMin = 0.55f;
        scaleMax = 1.05f;
        switch ((seed >> 7) % 3) {
            case 0: tint = sf::Color(96, 74, 44); break;
            case 1: tint = sf::Color(78, 62, 38); break;
            default: tint = sf::Color(110, 88, 52); break;
        }
    } else if (type == 7) {
        // Grass tuft: fills dense grassy ground between larger props.
        sourceRect = VisualConfig::DECOR_FERN;
        scaleMin = 0.6f;
        scaleMax = 1.0f;
        tint = ((seed >> 9) % 2 == 0) ? sf::Color(150, 200, 140) : sf::Color(120, 180, 115);
    }

    sprite.setTextureRect(sourceRect);
    sprite.setOrigin(sourceRect.width / 2.f, static_cast<float>(sourceRect.height));
    sprite.setPosition(x, y);
    sprite.setColor(tint);

    float scaleVar = SeedManager::getRandomFloat(seed, scaleMin, scaleMax);
    sprite.setScale(scaleVar, scaleVar);

    collisionBounds = sf::FloatRect(x - (sourceRect.width * scaleVar) / 2.f, y - (sourceRect.height * scaleVar), sourceRect.width * scaleVar, sourceRect.height * scaleVar);
}

void Decoration::update(float) {}

void Decoration::draw(sf::RenderTarget& target) const {
    target.draw(sprite);
}

sf::FloatRect Decoration::getBounds() const { return collisionBounds; }

sf::Vector2f Decoration::getOrigin() const { return sprite.getOrigin(); }