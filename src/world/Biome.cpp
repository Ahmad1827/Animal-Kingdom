#include "world/Biome.h"
#include <cmath>
#include <algorithm>

static float sampleMacroNoise(float x, float freq, float seedOffset) {
    float v1 = std::sin(x * freq + seedOffset);
    float v2 = std::sin(x * freq * 2.13f + seedOffset * 1.47f) * 0.5f;
    float v3 = std::cos(x * freq * 4.37f + seedOffset * 2.11f) * 0.25f;
    return (v1 + v2 + v3) / 1.75f;
}

EnvironmentalMetrics Biome::getMetrics(float worldX, uint32_t worldSeed) {
    float s1 = static_cast<float>((worldSeed * 1013904223u) % 10000);
    float s2 = static_cast<float>((worldSeed * 1664525u) % 10000);
    float s3 = static_cast<float>((worldSeed * 214013u) % 10000);

    float centerDistance = std::abs(worldX - 1000.f);
    float jungleBias = 0.0f;
    if (centerDistance <= 6000.f) {
        jungleBias = 1.0f;
    } else if (centerDistance <= 9500.f) {
        jungleBias = 1.0f - ((centerDistance - 6000.f) / 3500.f);
    }

    float rawTemp = sampleMacroNoise(worldX, 0.000035f, s1);
    float rawMoist = sampleMacroNoise(worldX, 0.00004f, s2);
    float rawElev = sampleMacroNoise(worldX, 0.000055f, s3);

    EnvironmentalMetrics m;
    m.temperature = std::clamp((0.5f + rawTemp * 0.35f) * (1.0f - jungleBias) + jungleBias * 0.88f, 0.0f, 1.0f);
    m.moisture    = std::clamp((0.5f + rawMoist * 0.35f) * (1.0f - jungleBias) + jungleBias * 0.98f, 0.0f, 1.0f);
    m.elevation   = std::clamp((0.5f + rawElev * 0.35f) * (1.0f - jungleBias) + jungleBias * 0.15f, 0.0f, 1.0f);
    m.fertility   = std::clamp(m.moisture * 0.7f + (1.0f - m.elevation) * 0.3f, 0.0f, 1.0f);

    return m;
}

BiomeWeights Biome::getWeights(float worldX, uint32_t worldSeed) {
    EnvironmentalMetrics m = getMetrics(worldX, worldSeed);

    struct Anchor {
        float temp;
        float moist;
        float elev;
    };

    static const Anchor anchors[] = {
        {0.85f, 0.95f, 0.15f},
        {0.45f, 0.45f, 0.40f},
        {0.85f, 0.12f, 0.30f},
        {0.40f, 0.35f, 0.70f},
        {0.25f, 0.15f, 0.90f}
    };

    BiomeWeights bw;
    float sum = 0.0f;
    const float sigmaSq = 0.08f;

    for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
        float dt = m.temperature - anchors[i].temp;
        float dm = m.moisture - anchors[i].moist;
        float de = m.elevation - anchors[i].elev;

        float distSq = dt * dt + dm * dm + de * de;
        float w = std::exp(-distSq / (2.0f * sigmaSq));
        bw.weights[i] = w;
        sum += w;
    }

    if (sum > 0.00001f) {
        for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
            bw.weights[i] /= sum;
        }
    } else {
        bw.weights[static_cast<int>(BiomeType::Jungle)] = 1.0f;
    }

    return bw;
}

BiomeType Biome::determineRegionAtWorldX(float worldX, uint32_t worldSeed) {
    return getWeights(worldX, worldSeed).getDominantBiome();
}

BiomeType Biome::determineRegion(int chunkX, uint32_t worldSeed) {
    return determineRegionAtWorldX(chunkX * 2000.f + 1000.f, worldSeed);
}

BiomeProperties Biome::getProperties(BiomeType type) {
    BiomeProperties p;
    p.type = type;

    switch (type) {
        case BiomeType::Jungle:
            p.name = "Tropical Jungle";
            p.vegetationMode = "JUNGLE TREES";
            p.groundColor = sf::Color(18, 62, 22);
            p.undergroundColor = sf::Color(24, 15, 8);
            p.grassTipColor = sf::Color(36, 105, 30);
            p.debugColor = sf::Color(0, 140, 45, 100);
            p.minTreeSpacing = 60.f;
            p.maxTreeSpacing = 95.f;
            p.treeDensity = 1.0f;
            p.clusterProbability = 0.90f;
            p.minClusterSize = 2;
            p.maxClusterSize = 4;
            p.satelliteOffsetMin = 25.f;
            p.satelliteOffsetMax = 45.f;
            p.treeScaleMin = 0.95f;
            p.treeScaleMax = 1.45f;
            p.treeWidthBase = 85.f;
            p.branchCountMin = 3;
            p.branchCountMax = 6;
            p.branchVerticalSpacing = 40.f;
            p.canopyBaseRadius = 100.f;
            p.decorationDensity = 40;
            p.primaryDecorType = 2;
            p.secondaryDecorType = 0;
            break;

        case BiomeType::Field:
            p.name = "Flower Field";
            p.vegetationMode = "FIELD FLOWERS";
            p.groundColor = sf::Color(72, 140, 42);
            p.undergroundColor = sf::Color(60, 44, 22);
            p.grassTipColor = sf::Color(110, 180, 60);
            p.debugColor = sf::Color(210, 235, 120, 100);
            p.minTreeSpacing = 500.f;
            p.maxTreeSpacing = 900.f;
            p.treeDensity = 0.08f;
            p.clusterProbability = 0.15f;
            p.minClusterSize = 1;
            p.maxClusterSize = 2;
            p.satelliteOffsetMin = 50.f;
            p.satelliteOffsetMax = 80.f;
            p.treeScaleMin = 0.65f;
            p.treeScaleMax = 0.95f;
            p.treeWidthBase = 55.f;
            p.branchCountMin = 1;
            p.branchCountMax = 3;
            p.branchVerticalSpacing = 50.f;
            p.canopyBaseRadius = 45.f;
            p.decorationDensity = 24;
            p.primaryDecorType = 1;
            p.secondaryDecorType = 0;
            break;

        case BiomeType::Desert:
            p.name = "Arid Desert";
            p.vegetationMode = "YELLOW SAND";
            p.groundColor = sf::Color(160, 135, 80);
            p.undergroundColor = sf::Color(110, 84, 48);
            p.grassTipColor = sf::Color(180, 155, 95);
            p.debugColor = sf::Color(220, 160, 60, 100);
            p.minTreeSpacing = 1200.f;
            p.maxTreeSpacing = 2400.f;
            p.treeDensity = 0.01f;
            p.clusterProbability = 0.05f;
            p.minClusterSize = 1;
            p.maxClusterSize = 1;
            p.satelliteOffsetMin = 80.f;
            p.satelliteOffsetMax = 120.f;
            p.treeScaleMin = 0.55f;
            p.treeScaleMax = 0.80f;
            p.treeWidthBase = 45.f;
            p.branchCountMin = 1;
            p.branchCountMax = 2;
            p.branchVerticalSpacing = 40.f;
            p.canopyBaseRadius = 35.f;
            p.decorationDensity = 14;
            p.primaryDecorType = 3;
            p.secondaryDecorType = 4;
            break;

        case BiomeType::Hills:
            p.name = "Rolling Hills";
            p.vegetationMode = "ROCKY HILLS";
            p.groundColor = sf::Color(52, 115, 38);
            p.undergroundColor = sf::Color(48, 36, 22);
            p.grassTipColor = sf::Color(80, 155, 54);
            p.debugColor = sf::Color(120, 180, 70, 100);
            p.minTreeSpacing = 160.f;
            p.maxTreeSpacing = 320.f;
            p.treeDensity = 0.45f;
            p.clusterProbability = 0.35f;
            p.minClusterSize = 1;
            p.maxClusterSize = 3;
            p.satelliteOffsetMin = 40.f;
            p.satelliteOffsetMax = 70.f;
            p.treeScaleMin = 0.75f;
            p.treeScaleMax = 1.15f;
            p.treeWidthBase = 65.f;
            p.branchCountMin = 2;
            p.branchCountMax = 4;
            p.branchVerticalSpacing = 45.f;
            p.canopyBaseRadius = 65.f;
            p.decorationDensity = 20;
            p.primaryDecorType = 0;
            p.secondaryDecorType = 3;
            break;

        case BiomeType::Mountain:
            p.name = "Highland Mountain";
            p.vegetationMode = "MOUNTAIN PEAKS";
            p.groundColor = sf::Color(100, 105, 95);
            p.undergroundColor = sf::Color(55, 50, 45);
            p.grassTipColor = sf::Color(125, 130, 120);
            p.debugColor = sf::Color(150, 150, 150, 100);
            p.minTreeSpacing = 260.f;
            p.maxTreeSpacing = 550.f;
            p.treeDensity = 0.15f;
            p.clusterProbability = 0.20f;
            p.minClusterSize = 1;
            p.maxClusterSize = 2;
            p.satelliteOffsetMin = 60.f;
            p.satelliteOffsetMax = 90.f;
            p.treeScaleMin = 0.70f;
            p.treeScaleMax = 1.05f;
            p.treeWidthBase = 55.f;
            p.branchCountMin = 2;
            p.branchCountMax = 3;
            p.branchVerticalSpacing = 50.f;
            p.canopyBaseRadius = 55.f;
            p.decorationDensity = 22;
            p.primaryDecorType = 3;
            p.secondaryDecorType = 4;
            break;
    }
    return p;
}

static sf::Color blendColor(const std::vector<sf::Color>& colors, const BiomeWeights& bw) {
    float r = 0.f, g = 0.f, b = 0.f, a = 0.f;
    for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
        float w = bw.weights[i];
        r += colors[i].r * w;
        g += colors[i].g * w;
        b += colors[i].b * w;
        a += colors[i].a * w;
    }
    return sf::Color(
        static_cast<sf::Uint8>(std::clamp(r, 0.f, 255.f)),
        static_cast<sf::Uint8>(std::clamp(g, 0.f, 255.f)),
        static_cast<sf::Uint8>(std::clamp(b, 0.f, 255.f)),
        static_cast<sf::Uint8>(std::clamp(a, 0.f, 255.f))
    );
}

sf::Color Biome::getBlendedGroundColor(float worldX, uint32_t worldSeed) {
    BiomeWeights bw = getWeights(worldX, worldSeed);
    std::vector<sf::Color> colors;
    for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
        colors.push_back(getProperties(static_cast<BiomeType>(i)).groundColor);
    }
    return blendColor(colors, bw);
}

sf::Color Biome::getBlendedUndergroundColor(float worldX, uint32_t worldSeed) {
    BiomeWeights bw = getWeights(worldX, worldSeed);
    std::vector<sf::Color> colors;
    for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
        colors.push_back(getProperties(static_cast<BiomeType>(i)).undergroundColor);
    }
    return blendColor(colors, bw);
}

sf::Color Biome::getBlendedGrassTipColor(float worldX, uint32_t worldSeed) {
    BiomeWeights bw = getWeights(worldX, worldSeed);
    std::vector<sf::Color> colors;
    for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
        colors.push_back(getProperties(static_cast<BiomeType>(i)).grassTipColor);
    }
    return blendColor(colors, bw);
}

BiomeProperties Biome::getBlendedProperties(float worldX, uint32_t worldSeed) {
    BiomeWeights bw = getWeights(worldX, worldSeed);
    BiomeType dom = bw.getDominantBiome();
    BiomeProperties result = getProperties(dom);

    result.groundColor = getBlendedGroundColor(worldX, worldSeed);
    result.undergroundColor = getBlendedUndergroundColor(worldX, worldSeed);
    result.grassTipColor = getBlendedGrassTipColor(worldX, worldSeed);

    float curvedWeights[static_cast<int>(BiomeType::Count)];
    float curvedSum = 0.0f;
    for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
        curvedWeights[i] = std::pow(bw.weights[i], 4.0f);
        curvedSum += curvedWeights[i];
    }
    if (curvedSum > 0.00001f) {
        for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
            curvedWeights[i] /= curvedSum;
        }
    } else {
        curvedWeights[static_cast<int>(dom)] = 1.0f;
    }

    result.minTreeSpacing = 0.f;
    result.maxTreeSpacing = 0.f;
    result.treeDensity = 0.f;
    result.clusterProbability = 0.f;
    result.treeScaleMin = 0.f;
    result.treeScaleMax = 0.f;
    result.treeWidthBase = 0.f;
    result.canopyBaseRadius = 0.f;
    float decorSum = 0.f;

    for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
        BiomeProperties p = getProperties(static_cast<BiomeType>(i));
        float w = curvedWeights[i];
        result.minTreeSpacing += p.minTreeSpacing * w;
        result.maxTreeSpacing += p.maxTreeSpacing * w;
        result.treeDensity += p.treeDensity * w;
        result.clusterProbability += p.clusterProbability * w;
        result.treeScaleMin += p.treeScaleMin * w;
        result.treeScaleMax += p.treeScaleMax * w;
        result.treeWidthBase += p.treeWidthBase * w;
        result.canopyBaseRadius += p.canopyBaseRadius * w;
        decorSum += static_cast<float>(p.decorationDensity) * w;
    }

    result.decorationDensity = std::max(10, static_cast<int>(std::round(decorSum)));
    return result;
}