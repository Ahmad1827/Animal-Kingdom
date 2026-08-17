#pragma once
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

enum class BiomeType {
    Jungle = 0,
    Field = 1,
    Desert = 2,
    Hills = 3,
    Mountain = 4,
    Count = 5
};

using RegionType = BiomeType;

struct EnvironmentalMetrics {
    float temperature;
    float moisture;
    float elevation;
    float fertility;
};

struct BiomeWeights {
    float weights[static_cast<int>(BiomeType::Count)];

    float get(BiomeType type) const {
        return weights[static_cast<int>(type)];
    }

    BiomeType getDominantBiome() const {
        int bestIdx = 0;
        float maxW = -1.0f;
        for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
            if (weights[i] > maxW) {
                maxW = weights[i];
                bestIdx = i;
            }
        }
        return static_cast<BiomeType>(bestIdx);
    }
};

struct BiomeProperties {
    std::string name;
    BiomeType type;
    sf::Color groundColor;
    sf::Color undergroundColor;
    sf::Color grassTipColor;
    sf::Color debugColor;
    
    float minTreeSpacing;
    float maxTreeSpacing;
    float treeDensity;
    float clusterProbability;
    float treeScaleMin;
    float treeScaleMax;
    float treeWidthBase;

    int branchCountMin;
    int branchCountMax;
    float branchVerticalSpacing;
    float canopyBaseRadius;
    int decorationDensity;
    int primaryDecorType;
    int secondaryDecorType;
};

class Biome {
public:
    static EnvironmentalMetrics getMetrics(float worldX, uint32_t worldSeed);
    static BiomeWeights getWeights(float worldX, uint32_t worldSeed);
    static BiomeType determineRegion(int chunkX, uint32_t worldSeed);
    static BiomeType determineRegionAtWorldX(float worldX, uint32_t worldSeed);
    static BiomeProperties getProperties(BiomeType type);
    static BiomeProperties getBlendedProperties(float worldX, uint32_t worldSeed);
    static sf::Color getBlendedGroundColor(float worldX, uint32_t worldSeed);
    static sf::Color getBlendedUndergroundColor(float worldX, uint32_t worldSeed);
    static sf::Color getBlendedGrassTipColor(float worldX, uint32_t worldSeed);
};