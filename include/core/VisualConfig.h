#pragma once
#include <SFML/Graphics.hpp>

namespace VisualConfig {
    const sf::IntRect DECOR_TREE(17, 12, 73, 100);
    const sf::IntRect DECOR_BUSH(114, 96, 35, 16);
    const sf::IntRect DECOR_BUSH_SMALL(51, 80, 16, 15);
    const sf::IntRect DECOR_FERN(12, 110, 15, 11);
    const sf::IntRect DECOR_ROCK(177, 92, 27, 20);
    const sf::IntRect DECOR_ROCK_SMALL(91, 82, 9, 8);
    const sf::IntRect DECOR_ROOT(106, 110, 10, 5);

    const sf::IntRect TREEFIXED_1(0, 0, 80, 200);
    const sf::IntRect TREEFIXED_2(80, 0, 120, 260);
    const sf::IntRect TREEFIXED_3(200, 0, 170, 330);
    const sf::IntRect TREEFIXED_4(370, 0, 240, 410);
    const sf::IntRect TREEFIXED_5(610, 0, 330, 470);

    const sf::IntRect TILE_SURFACE(0, 0, 32, 32);
    const sf::IntRect TILE_UNDERGROUND(64, 0, 32, 32);

    const float PARALLAX_FAR_SPEED = 0.05f;
    const float PARALLAX_MID_SPEED = 0.1f;
    const float PARALLAX_NEAR_SPEED = 0.15f;

    const float TERRAIN_TEXTURE_SCALE = 32.f;
}