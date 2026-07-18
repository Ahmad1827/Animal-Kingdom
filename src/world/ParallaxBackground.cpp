#include "world/ParallaxBackground.h"
#include <cmath>

ParallaxBackground::ParallaxBackground() {
    auto addLayer = [&](sf::Color color, float speed, float yOff, float height) {
        Layer l;
        l.speedMultiplier = speed;
        l.textureWidth = 3000.f;
        l.yOffset = yOff;
        
        l.shape1.setSize({l.textureWidth, height});
        l.shape1.setFillColor(color);

        l.shape2.setSize({l.textureWidth, height});
        l.shape2.setFillColor(color);
        
        layers.push_back(l);
    };

    // Clean, readable silhouettes that do not compete with gameplay
    addLayer(sf::Color(15, 20, 25), 0.00f, -1500.f, 4000.f); // Very Dark Sky
    addLayer(sf::Color(20, 28, 35), 0.03f, -500.f,  2000.f); // Far Background Mist
    addLayer(sf::Color(25, 35, 40), 0.08f, -200.f,  2000.f); // Distant Jungle Shapes
    addLayer(sf::Color(30, 45, 45), 0.15f, -50.f,   2000.f); // Mid Jungle Block
}

void ParallaxBackground::update(float cameraX, float cameraY) {
    for (auto& layer : layers) {
        if (layer.speedMultiplier == 0.f) {
            layer.shape1.setPosition(cameraX - layer.textureWidth / 2.f, cameraY + layer.yOffset);
            continue;
        }

        float offset = cameraX * layer.speedMultiplier;
        float startX = cameraX - std::fmod(cameraX - offset, layer.textureWidth);
        if (startX > cameraX) startX -= layer.textureWidth;
        
        float adjustedY = cameraY * (1.0f - (layer.speedMultiplier * 0.15f)) + layer.yOffset;

        layer.shape1.setPosition(startX - layer.textureWidth, adjustedY);
        layer.shape2.setPosition(startX, adjustedY);
    }
}

void ParallaxBackground::draw(sf::RenderWindow& window) const {
    for (const auto& layer : layers) {
        if (layer.speedMultiplier == 0.f) {
            window.draw(layer.shape1);
        } else {
            window.draw(layer.shape1);
            window.draw(layer.shape2);
        }
    }
}