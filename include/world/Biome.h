#pragma once
#include <string>
#include <SFML/Graphics.hpp>

enum class BiomeType {
    Jungle = 0,
    Field = 1,
    Desert = 2
};

using RegionType = BiomeType;

struct BiomeProperties {
    std::string name;
    BiomeType type;
    std::string vegetationMode;
    sf::Color groundColor;
    sf::Color undergroundColor;
    sf::Color grassTipColor;
    sf::Color debugColor;
    float minTreeSpacing;
    float maxTreeSpacing;
};

class Biome {
public:
    static BiomeType determineRegionAtWorldX(float worldX, uint32_t worldSeed);
    static BiomeType determineRegion(int chunkX, uint32_t worldSeed);
    static BiomeProperties getProperties(BiomeType type);
};