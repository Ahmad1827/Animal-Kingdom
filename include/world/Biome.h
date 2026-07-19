#pragma once
#include <string>
#include <SFML/Graphics.hpp>

enum class RegionType {
    YoungForest,
    OldGrowth,
    Clearing,
    RockyArea,
    DenseJungle
};

struct BiomeProperties {
    std::string name;
    sf::Color groundColor;
    sf::Color undergroundColor;
    sf::Color debugColor;
    
    // Tuning Constants
    float minTreeSpacing;
    float maxTreeSpacing;
    float treeSizeMultiplier;
    float treeWidthBase;
    int branchCountMin;
    int branchCountMax;
    float branchVerticalSpacing;
    float canopyBaseRadius;
    int decorationDensity;
    int preferredDecorationType; // 0=Grass, 1=Flower, 2=Fern, 3=Rock, 4=Log
};

class Biome {
public:
    static RegionType determineRegion(int chunkX, uint32_t worldSeed);
    static BiomeProperties getProperties(RegionType type);
};