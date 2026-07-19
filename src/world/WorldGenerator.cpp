#include "world/WorldGenerator.h"
#include "world/TerrainGenerator.h"
#include "world/SeedManager.h"
#include <cmath>

std::vector<Tree> WorldGenerator::generateTrees(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& biome) {
    std::vector<Tree> trees;
    float currentX = startX;

    while (currentX < startX + width) {
        // Sample exact configuration based on region transitioning
        RegionType currentReg = Biome::determineRegion(currentX / 2000.f, worldSeed); // approximate chunk div
        BiomeProperties config = Biome::getProperties(currentReg);

        // Intentional Spacing & Exclusion Rules
        currentX += SeedManager::getRandomFloat(chunkSeed, config.minTreeSpacing, config.maxTreeSpacing);
        if (currentX >= startX + width) break;

        // Terrain Alignment
        float terrainY = TerrainGenerator::getTerrainHeight(currentX, worldSeed);

        // Dimensional Scaling
        float trunkWidth = config.treeWidthBase * SeedManager::getRandomFloat(chunkSeed, 0.8f, 1.2f);
        float trunkHeight = SeedManager::getRandomFloat(chunkSeed, 400.f, 900.f) * config.treeSizeMultiplier;
        sf::Color trunkColor(70 + (chunkSeed % 20), 50 + (chunkSeed % 10), 30);
        
        Tree tree(currentX, terrainY, trunkWidth, trunkHeight, trunkColor);

        // Readable Climbing Path Generation
        int branchCount = SeedManager::getRandomInt(chunkSeed, config.branchCountMin, config.branchCountMax);
        float currentBranchY = 180.f; // Start high enough above ground
        bool rightSide = (chunkSeed % 2 == 0); // Alternate path

        for (int i = 0; i < branchCount; ++i) {
            if (currentBranchY > trunkHeight - 100.f) break; // Don't clip through canopy

            float bWidth = SeedManager::getRandomFloat(chunkSeed, 150.f, 300.f) * config.treeSizeMultiplier;
            tree.addBranch(currentBranchY, bWidth, rightSide, trunkColor);
            
            // Add decorative vines exclusively to branch tips for readability
            if (chunkSeed % 100 < 60) {
                float vX = rightSide ? bWidth - 10.f : -10.f;
                tree.addVine(vX, currentBranchY, SeedManager::getRandomFloat(chunkSeed, 100.f, 250.f));
            }

            currentBranchY += config.branchVerticalSpacing * SeedManager::getRandomFloat(chunkSeed, 0.9f, 1.1f);
            rightSide = !rightSide; // Force alternation to guarantee jump path
            chunkSeed++; 
        }

        // Canopy Generation
        sf::Color leafColor(20 + (chunkSeed % 30), 100 + (chunkSeed % 50), 30);
        float cRadius = config.canopyBaseRadius * SeedManager::getRandomFloat(chunkSeed, 0.8f, 1.2f);
        int clusters = static_cast<int>(config.treeSizeMultiplier * 3) + SeedManager::getRandomInt(chunkSeed, 1, 3);
        tree.buildCanopy(chunkSeed, cRadius, trunkHeight - 50.f, leafColor, clusters);

        tree.initDynamicMesh();
        trees.push_back(std::move(tree));
    }
    return trees;
}

std::vector<Decoration> WorldGenerator::generateDecorations(float startX, float width, uint32_t chunkSeed, uint32_t worldSeed, const BiomeProperties& biome) {
    std::vector<Decoration> decorations;
    float currentX = startX;

    while (currentX < startX + width) {
        RegionType currentReg = Biome::determineRegion(currentX / 2000.f, worldSeed);
        BiomeProperties config = Biome::getProperties(currentReg);

        currentX += SeedManager::getRandomFloat(chunkSeed, 30.f, 150.f);
        if (currentX >= startX + width) break;

        // Terrain Alignment
        float terrainY = TerrainGenerator::getTerrainHeight(currentX, worldSeed);
        
        // Skip underwater placements (water level is 750)
        if (terrainY > 745.f) continue;

        // Determine specific type based on region preferences
        int type = config.preferredDecorationType;
        if (chunkSeed % 100 < 30) {
            type = SeedManager::getRandomInt(chunkSeed, 0, 4); // 30% chance for random chaos
        }

        static sf::Texture dummyTex; 
        if (dummyTex.getSize().x == 0) {
            dummyTex.create(32, 32);
        }
        decorations.push_back(Decoration(currentX, terrainY, type, chunkSeed, dummyTex));
        chunkSeed++;
    }
    return decorations;
}