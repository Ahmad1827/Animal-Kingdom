#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>

class TerrainGenerator {
public:
    static float getTerrainHeight(float x, uint32_t worldSeed);
    static sf::VertexArray generateSurfaceMesh(const sf::FloatRect& chunkBounds, float resolution, uint32_t worldSeed);
    static sf::VertexArray generateUndergroundMesh(const sf::FloatRect& chunkBounds, float resolution, uint32_t worldSeed, sf::Color undergroundColor);
};