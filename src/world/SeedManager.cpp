#include "world/SeedManager.h"
#include <cmath>

uint32_t SeedManager::getChunkSeed(uint32_t worldSeed, int chunkX) {
    uint32_t hash = worldSeed;
    hash ^= (uint32_t)chunkX + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash;
}

float SeedManager::getRandomFloat(uint32_t& seed, float min, float max) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    float normalized = static_cast<float>(seed) / static_cast<float>(0xFFFFFFFF);
    return min + normalized * (max - min);
}

int SeedManager::getRandomInt(uint32_t& seed, int min, int max) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return min + (seed % (max - min + 1));
}

uint32_t SeedManager::hashPosition(int32_t gridPos, uint32_t seed) {
    uint32_t h = static_cast<uint32_t>(gridPos) * 0x9E3779B1u;
    h ^= seed * 0x85EBCA6Bu;
    h ^= h >> 15; h *= 0x2C1B3C6Du;
    h ^= h >> 12; h *= 0x297A2D39u;
    h ^= h >> 15;
    return h;
}

float SeedManager::valueNoise(float x, uint32_t seed, float frequency) {
    float sx = x * frequency;
    float fl = std::floor(sx);
    int32_t i0 = static_cast<int32_t>(fl);
    float t = sx - fl;

    float a = static_cast<float>(hashPosition(i0, seed)) / 4294967295.0f;
    float b = static_cast<float>(hashPosition(i0 + 1, seed)) / 4294967295.0f;

    // Smoothstep so patches blend instead of stepping between lattice points.
    float s = t * t * (3.0f - 2.0f * t);
    return a + (b - a) * s;
}

float SeedManager::fbm(float x, uint32_t seed, float baseFrequency, int octaves) {
    if (octaves < 1) octaves = 1;
    float sum = 0.0f;
    float amp = 1.0f;
    float totalAmp = 0.0f;
    float freq = baseFrequency;

    for (int o = 0; o < octaves; ++o) {
        sum += valueNoise(x, seed + static_cast<uint32_t>(o) * 0x9E37u, freq) * amp;
        totalAmp += amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }
    return (totalAmp > 0.0f) ? (sum / totalAmp) : 0.0f;
}