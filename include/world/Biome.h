#pragma once
#include <string>
#include <SFML/Graphics.hpp>

enum class BiomeType {
    Jungle = 0,
    Field = 1,
    Desert = 2
};

using RegionType = BiomeType;

struct BiomeTransitionInfo {
    BiomeType currentBiome;
    BiomeType nextBiome;
    float blendFactor;
    float jungleWeight;
    float fieldWeight;
    float desertWeight;
};

struct BiomeProperties {
    std::string name;
    BiomeType type;
    std::string vegetationMode;
    sf::Color groundColor;
    sf::Color undergroundColor;
    sf::Color subsoilColor;
    sf::Color grassTipColor;
    sf::Color grassBaseColor;
    sf::Color debugColor;
    float minTreeSpacing;
    float maxTreeSpacing;
};

class Biome {
public:
    static BiomeTransitionInfo getTransitionInfo(float worldX);
    static BiomeType determineRegionAtWorldX(float worldX, uint32_t worldSeed = 0);
    static BiomeType determineRegion(int chunkX, uint32_t worldSeed = 0);
    static BiomeProperties getProperties(BiomeType type);
    static sf::Color getBlendedGroundColor(float worldX, uint32_t worldSeed = 0);
    static sf::Color getBlendedUndergroundColor(float worldX, uint32_t worldSeed = 0);
    static sf::Color getBlendedSubsoilColor(float worldX, uint32_t worldSeed = 0);
    static sf::Color getBlendedGrassTipColor(float worldX, uint32_t worldSeed = 0);
    static sf::Color getBlendedGrassBaseColor(float worldX, uint32_t worldSeed = 0);
};