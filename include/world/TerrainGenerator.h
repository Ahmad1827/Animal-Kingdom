#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>

class TerrainGenerator {
public:
    static float getTerrainHeight(float x, uint32_t worldSeed);
    static sf::VertexArray generateTerrainMesh(float startX, float width, float resolution, uint32_t worldSeed, sf::Color topColor, sf::Color bottomColor);
};