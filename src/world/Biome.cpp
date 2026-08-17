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
    float jungleBias = std::max(0.0f, 1.0f - (centerDistance / 6000.f));

    float rawTemp = sampleMacroNoise(worldX, 0.000035f, s1);
    float rawMoist = sampleMacroNoise(worldX, 0.000045f, s2);
    float rawElev = sampleMacroNoise(worldX, 0.000065f, s3);

    EnvironmentalMetrics m;
    m.temperature = std::clamp(0.5f + rawTemp * 0.45f + jungleBias * 0.25f, 0.0f, 1.0f);
    m.moisture = std::clamp(0.5f + rawMoist * 0.45f + jungleBias * 0.35f, 0.0f, 1.0f);
    m.elevation = std::clamp(0.5f + rawElev * 0.45f - jungleBias * 0.15f, 0.0f, 1.0f);
    m.fertility = std::clamp(m.moisture * 0.6f + (1.0f - m.temperature * 0.3f), 0.0f, 1.0f);

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
        {0.75f, 0.85f, 0.30f}, // Jungle
        {0.45f, 0.50f, 0.40f}, // Field
        {0.85f, 0.15f, 0.35f}, // Desert
        {0.40f, 0.35f, 0.70f}, // Hills
        {0.25f, 0.20f, 0.90f}  // Mountain
    };

    BiomeWeights bw;
    float sum = 0.0f;
    const float sigmaSq = 0.14f;

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
            p.groundColor = sf::Color(28, 92, 32);
            p.undergroundColor = sf::Color(35, 22, 12);
            p.grassTipColor = sf::Color(55, 140, 40);
            p.debugColor = sf::Color(0, 140, 45, 100);
            p.minTreeSpacing = 65.f;
            p.maxTreeSpacing = 135.f;
            p.treeDensity = 0.95f;
            p.clusterProbability = 0.75f;
            p.treeScaleMin = 0.95f;
            p.treeScaleMax = 1.45f;
            p.treeWidthBase = 80.f;
            p.branchCountMin = 4;
            p.branchCountMax = 8;
            p.branchVerticalSpacing = 140.f;
            p.canopyBaseRadius = 100.f;
            p.decorationDensity = 24;
            p.primaryDecorType = 2;
            p.secondaryDecorType = 0;
            break;

        case BiomeType::Field:
            p.name = "Flower Field";
            p.groundColor = sf::Color(78, 145, 48);
            p.undergroundColor = sf::Color(65, 48, 26);
            p.grassTipColor = sf::Color(115, 185, 65);
            p.debugColor = sf::Color(210, 235, 120, 100);
            p.minTreeSpacing = 380.f;
            p.maxTreeSpacing = 750.f;
            p.treeDensity = 0.15f;
            p.clusterProbability = 0.20f;
            p.treeScaleMin = 0.70f;
            p.treeScaleMax = 1.00f;
            p.treeWidthBase = 55.f;
            p.branchCountMin = 1;
            p.branchCountMax = 3;
            p.branchVerticalSpacing = 100.f;
            p.canopyBaseRadius = 50.f;
            p.decorationDensity = 18;
            p.primaryDecorType = 1;
            p.secondaryDecorType = 0;
            break;

        case BiomeType::Desert:
            p.name = "Arid Desert";
            p.groundColor = sf::Color(165, 140, 85);
            p.undergroundColor = sf::Color(115, 88, 52);
            p.grassTipColor = sf::Color(185, 160, 100);
            p.debugColor = sf::Color(220, 160, 60, 100);
            p.minTreeSpacing = 900.f;
            p.maxTreeSpacing = 1800.f;
            p.treeDensity = 0.03f;
            p.clusterProbability = 0.05f;
            p.treeScaleMin = 0.60f;
            p.treeScaleMax = 0.85f;
            p.treeWidthBase = 45.f;
            p.branchCountMin = 1;
            p.branchCountMax = 2;
            p.branchVerticalSpacing = 90.f;
            p.canopyBaseRadius = 35.f;
            p.decorationDensity = 12;
            p.primaryDecorType = 3;
            p.secondaryDecorType = 4;
            break;

        case BiomeType::Hills:
            p.name = "Rolling Hills";
            p.groundColor = sf::Color(55, 120, 42);
            p.undergroundColor = sf::Color(52, 40, 25);
            p.grassTipColor = sf::Color(85, 160, 58);
            p.debugColor = sf::Color(120, 180, 70, 100);
            p.minTreeSpacing = 160.f;
            p.maxTreeSpacing = 320.f;
            p.treeDensity = 0.55f;
            p.clusterProbability = 0.40f;
            p.treeScaleMin = 0.80f;
            p.treeScaleMax = 1.15f;
            p.treeWidthBase = 65.f;
            p.branchCountMin = 2;
            p.branchCountMax = 5;
            p.branchVerticalSpacing = 120.f;
            p.canopyBaseRadius = 65.f;
            p.decorationDensity = 16;
            p.primaryDecorType = 0;
            p.secondaryDecorType = 3;
            break;

        case BiomeType::Mountain:
            p.name = "Highland Mountain";
            p.groundColor = sf::Color(105, 110, 100);
            p.undergroundColor = sf::Color(60, 55, 50);
            p.grassTipColor = sf::Color(130, 135, 125);
            p.debugColor = sf::Color(150, 150, 150, 100);
            p.minTreeSpacing = 260.f;
            p.maxTreeSpacing = 550.f;
            p.treeDensity = 0.25f;
            p.clusterProbability = 0.20f;
            p.treeScaleMin = 0.70f;
            p.treeScaleMax = 1.05f;
            p.treeWidthBase = 55.f;
            p.branchCountMin = 2;
            p.branchCountMax = 4;
            p.branchVerticalSpacing = 150.f;
            p.canopyBaseRadius = 55.f;
            p.decorationDensity = 20;
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
        float w = bw.weights[i];
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

    result.decorationDensity = std::max(6, static_cast<int>(std::round(decorSum)));
    return result;
}