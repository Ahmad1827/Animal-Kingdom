#include "world/ParallaxBackground.h"
#include <cmath>

ParallaxBackground::ParallaxBackground() {
    auto addLayer = [&](sf::Color color, float speed) {
        Layer l;
        l.speedMultiplier = speed;
        l.textureWidth = 2000.f;
        
        l.shape1.setSize({l.textureWidth, 1000.f});
        l.shape1.setFillColor(color);
        l.shape1.setPosition(0.f, -200.f);

        l.shape2.setSize({l.textureWidth, 1000.f});
        l.shape2.setFillColor(color);
        l.shape2.setPosition(l.textureWidth, -200.f);
        
        layers.push_back(l);
    };

    addLayer(sf::Color(135, 206, 235), 0.0f); // Sky (Fixed)
    addLayer(sf::Color(100, 140, 160), 0.2f); // Mountains
    addLayer(sf::Color(60, 100, 60), 0.5f);   // Far Forest
}

void ParallaxBackground::update(float cameraX) {
    for (auto& layer : layers) {
        if (layer.speedMultiplier == 0.f) {
            layer.shape1.setPosition(cameraX - layer.textureWidth / 2.f, layer.shape1.getPosition().y);
            continue;
        }

        float offset = cameraX * layer.speedMultiplier;
        float startX = cameraX - std::fmod(cameraX - offset, layer.textureWidth);
        
        layer.shape1.setPosition(startX - layer.textureWidth, layer.shape1.getPosition().y);
        layer.shape2.setPosition(startX, layer.shape2.getPosition().y);
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