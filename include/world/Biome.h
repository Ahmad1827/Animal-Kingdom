#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string>

enum class RegionType {
    OldGrowth,
    Clearing,
    YoungForest,
    AncientGrove,
    DenseJungle
};

struct BiomeProperties {
    std::string name;
    int minTrees;
    int maxTrees;
    int minDecorations;
    int maxDecorations;
    float treeSizeMultiplier;
    int branchCountModifier;
    float canopyDensity;
    sf::Color groundColor;
    sf::Color undergroundColor;
    sf::Color trunkColorMin;
    sf::Color trunkColorMax;
    sf::Color leafColorMin;
    sf::Color leafColorMax;
    sf::Color debugColor;
};

class Biome {
public:
    static RegionType determineRegion(int chunkX, uint32_t worldSeed);
    static BiomeProperties getProperties(RegionType type);
};