#include "core/AssetManager.h"
#include <iostream>

sf::Texture& AssetManager::getTexture(const std::string& filepath, sf::Vector2u fallbackSize, sf::Color fallbackColor) {
    if (textures.find(filepath) == textures.end()) {
        sf::Texture tex;
        if (tex.loadFromFile(filepath)) {
            textures[filepath] = tex;
        } else {
            sf::Image fallbackImg;
            fallbackImg.create(fallbackSize.x, fallbackSize.y, fallbackColor);
            tex.loadFromImage(fallbackImg);
            textures[filepath] = tex;
        }
    }
    return textures[filepath];
}