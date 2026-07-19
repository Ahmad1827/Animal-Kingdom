#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include <memory>

class AssetManager {
private:
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures;
    sf::Texture placeholderTexture;

public:
    AssetManager();
    void loadTexture(const std::string& name, const std::string& filename, bool repeated = false);
    sf::Texture& getTexture(const std::string& name);
};