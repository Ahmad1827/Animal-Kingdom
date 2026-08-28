#pragma once
#include <SFML/Graphics.hpp>

namespace VisualConfig {
    const sf::IntRect DECOR_TREE(17, 12, 73, 100);
    const sf::IntRect DECOR_BUSH(114, 96, 35, 16);
    const sf::IntRect DECOR_ROCK(177, 92, 27, 20);

    const sf::IntRect DECOR_FERN(12, 110, 15, 11);
    const sf::IntRect DECOR_BUSH_SMALL(51, 80, 16, 15);
    const sf::IntRect DECOR_ROOT(106, 110, 10, 5);
    const sf::IntRect DECOR_ROCK_SMALL(91, 82, 9, 8);

    const sf::IntRect TREEFIXED_1(0, 0, 80, 200);
    const sf::IntRect TREEFIXED_2(80, 0, 120, 260);
    const sf::IntRect TREEFIXED_3(200, 0, 170, 330);
    const sf::IntRect TREEFIXED_4(370, 0, 240, 410);
    const sf::IntRect TREEFIXED_5(610, 0, 330, 470);

    const sf::IntRect TILE_SURFACE(0, 0, 32, 32);
    const sf::IntRect TILE_UNDERGROUND(64, 0, 32, 32);

    const sf::IntRect JUNGLE_GROUND_TOP_01(8, 94, 1479, 141);
    const sf::IntRect JUNGLE_GROUND_TOP_02(8, 377, 1479, 101);
    const sf::IntRect JUNGLE_GROUND_TOP_03(243, 1144, 1010, 63);
    const sf::IntRect JUNGLE_GROUND_TOP_05(1964, 843, 559, 121);

    const sf::IntRect JUNGLE_SOIL_01(19, 1223, 486, 227);
    const sf::IntRect JUNGLE_SOIL_02(505, 1223, 486, 227);
    const sf::IntRect JUNGLE_SOIL_03(991, 1223, 486, 227);

    const sf::IntRect JUNGLE_ROOT_01(665, 567, 184, 154);
    const sf::IntRect JUNGLE_ROOT_02(2125, 570, 219, 151);
    const sf::IntRect JUNGLE_ROOT_03(3652, 562, 168, 159);

    const sf::IntRect JUNGLE_FERN_01(2156, 116, 172, 119);
    const sf::IntRect JUNGLE_FERN_02(3666, 143, 150, 92);
    const sf::IntRect JUNGLE_FERN_04(5168, 153, 138, 82);

    const sf::IntRect JUNGLE_PLANT_01(9653, 1790, 146, 146);
    const sf::IntRect JUNGLE_PLANT_02(2177, 1820, 141, 116);
    const sf::IntRect JUNGLE_PLANT_03(6692, 1811, 75, 125);
    const sf::IntRect JUNGLE_PLANT_04(8154, 1805, 144, 131);

    const sf::IntRect JUNGLE_ROCK_01(2157, 1114, 174, 93);
    const sf::IntRect JUNGLE_ROCK_02(3639, 1086, 193, 121);
    const sf::IntRect JUNGLE_ROCK_03(5145, 1063, 200, 144);

    const sf::IntRect JUNGLE_LEAVES_01(621, 1610, 253, 83);
    const sf::IntRect JUNGLE_LEAVES_02(2136, 1593, 216, 100);
    const sf::IntRect JUNGLE_LEAVES_03(3652, 1616, 175, 77);

    const float PARALLAX_FAR_SPEED = 0.05f;
    const float PARALLAX_MID_SPEED = 0.1f;
    const float PARALLAX_NEAR_SPEED = 0.15f;

    const float TERRAIN_TEXTURE_SCALE = 32.f;
}