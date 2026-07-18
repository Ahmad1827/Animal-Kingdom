#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class TerrainGenerator {
public:
    static float getTerrainHeight(float x);
    static sf::VertexArray generateTerrainMesh(float startX, float width, float resolution);
};