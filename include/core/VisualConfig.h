#pragma once
#include <SFML/Graphics.hpp>

namespace VisualConfig {
    // Decors.png (measured exact bounds, not padding)
    const sf::IntRect DECOR_TREE(17, 12, 73, 100);
    const sf::IntRect DECOR_BUSH(114, 96, 35, 16);
    const sf::IntRect DECOR_ROCK(177, 92, 27, 20);

    // Tileset.png
    const sf::IntRect TILE_SURFACE(0, 0, 32, 32);
    const sf::IntRect TILE_UNDERGROUND(64, 0, 32, 32); // Fixed: was a rounded blob tile

    const float PARALLAX_FAR_SPEED = 0.05f;
    const float PARALLAX_MID_SPEED = 0.1f;
    const float PARALLAX_NEAR_SPEED = 0.15f;

    const float TERRAIN_TEXTURE_SCALE = 32.f;
}