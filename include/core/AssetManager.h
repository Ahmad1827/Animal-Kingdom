#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>

class AssetManager {
private:
    std::map<std::string, sf::Texture> textures;

public:
    sf::Texture& getTexture(const std::string& filepath, sf::Vector2u fallbackSize, sf::Color fallbackColor);
};