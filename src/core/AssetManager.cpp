#include "core/AssetManager.h"

AssetManager::AssetManager() {
    placeholderTexture.create(32, 32);
    sf::Uint8 pixels[32 * 32 * 4];
    for (int i = 0; i < 32 * 32 * 4; ++i) pixels[i] = 255;
    placeholderTexture.update(pixels);
}

void AssetManager::loadTexture(const std::string& name, const std::string& filename) {
    auto tex = std::make_unique<sf::Texture>();
    if (tex->loadFromFile(filename)) {
        textures[name] = std::move(tex);
    }
}

sf::Texture& AssetManager::getTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) return *(it->second);
    return placeholderTexture;
}