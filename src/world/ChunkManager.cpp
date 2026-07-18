#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"
#include <cmath>
#include <chrono>


ChunkManager::ChunkManager(uint32_t seed) : chunkWidth(3000.f), worldSeed(seed), currentChunkIdx(0) {}

void ChunkManager::update(const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler) {
    int minLoadIdx = static_cast<int>(std::floor(preloadBounds.left / chunkWidth));
    int maxLoadIdx = static_cast<int>(std::floor((preloadBounds.left + preloadBounds.width) / chunkWidth));

    // Process completed async chunks
    for (auto it = pendingChunks.begin(); it != pendingChunks.end();) {
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            auto newChunk = it->second.get();
            profiler.lastTerrainGenTime = newChunk->getTerrainGenTime();
            profiler.lastTreeGenTime = newChunk->getTreeGenTime();
            profiler.lastChunkGenTime = newChunk->getTotalGenTime();
            
            chunks[it->first] = std::move(newChunk);
            it = pendingChunks.erase(it);
        } else {
            ++it;
        }
    }

    // Request new chunks asynchronously
    for (int i = minLoadIdx; i <= maxLoadIdx; ++i) {
        if (chunks.find(i) == chunks.end() && pendingChunks.find(i) == pendingChunks.end()) {
            if (cachedChunks.find(i) != cachedChunks.end()) {
                chunks[i] = std::move(cachedChunks[i]);
                cachedChunks.erase(i);
            } else {
                uint32_t seed = worldSeed;
                float width = chunkWidth;
                pendingChunks[i] = std::async(std::launch::async, [i, width, seed]() {
                    return std::make_unique<Chunk>(i, width, seed);
                });
            }
        }
    }

    // Safely unload to cache
    for (auto it = chunks.begin(); it != chunks.end();) {
        float chunkLeft = it->first * chunkWidth;
        float chunkRight = chunkLeft + chunkWidth;

        if (chunkRight < unloadBounds.left || chunkLeft > unloadBounds.left + unloadBounds.width) {
            cachedChunks[it->first] = std::move(it->second);
            it = chunks.erase(it);
        } else {
            it->second->getTrees(); 
            ++it;
        }
    }
    
    currentChunkIdx = static_cast<int>(std::floor((preloadBounds.left + (preloadBounds.width / 2.f)) / chunkWidth));
    
    profiler.chunksLoaded = chunks.size();
    profiler.chunksGenerating = pendingChunks.size();
    profiler.chunksCached = cachedChunks.size();
}

int ChunkManager::getChunkIndexAt(float x) const {
    return static_cast<int>(std::floor(x / chunkWidth));
}

Chunk* ChunkManager::getChunk(int index) const {
    auto it = chunks.find(index);
    if (it != chunks.end()) return it->second.get();
    return nullptr;
}

void ChunkManager::draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const {
    ProfilerStats dummyProfiler;
    drawBackground(window, viewBounds, true, dummyProfiler);
    drawGeometry(window, viewBounds, dummyProfiler);
}

void ChunkManager::drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler) const {
    sf::FloatRect renderBounds = viewBounds;
    renderBounds.left -= 500.f;
    renderBounds.width += 1000.f;

    for (const auto& pair : chunks) {
        pair.second->drawBackground(window, renderBounds, showFoliage, profiler);
    }
}

void ChunkManager::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    sf::FloatRect renderBounds = viewBounds;
    renderBounds.left -= 500.f;
    renderBounds.width += 1000.f;

    for (const auto& pair : chunks) {
        pair.second->drawGeometry(window, renderBounds, profiler);
    }
}

void ChunkManager::drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, bool showBorders, bool showRegions, bool showHeatmaps) const {
    // Draw Camera Bounds Visualization
    sf::RectangleShape cam(sf::Vector2f(viewBounds.width, viewBounds.height));
    cam.setPosition(viewBounds.left, viewBounds.top);
    cam.setFillColor(sf::Color::Transparent);
    cam.setOutlineColor(sf::Color::White);
    cam.setOutlineThickness(4.f);
    window.draw(cam);

    sf::RectangleShape pre(sf::Vector2f(preloadBounds.width, preloadBounds.height));
    pre.setPosition(preloadBounds.left, preloadBounds.top);
    pre.setFillColor(sf::Color::Transparent);
    pre.setOutlineColor(sf::Color::Green);
    pre.setOutlineThickness(8.f);
    window.draw(pre);
    
    sf::RectangleShape unl(sf::Vector2f(unloadBounds.width, unloadBounds.height));
    unl.setPosition(unloadBounds.left, unloadBounds.top);
    unl.setFillColor(sf::Color::Transparent);
    unl.setOutlineColor(sf::Color::Red);
    unl.setOutlineThickness(12.f);
    window.draw(unl);

    for (const auto& pair : chunks) {
        float startX = pair.first * chunkWidth;
        
        if (showBorders) {
            sf::RectangleShape border({10.f, 4000.f});
            border.setPosition(startX, -1000.f);
            border.setFillColor(sf::Color(255, 0, 0, 150));
            window.draw(border);
        }
        
        if (showRegions) {
            BiomeProperties props = Biome::getProperties(pair.second->getRegionType());
            sf::RectangleShape regionOverlay({chunkWidth, 4000.f});
            regionOverlay.setPosition(startX, -1000.f);
            regionOverlay.setFillColor(props.debugColor);
            window.draw(regionOverlay);
        }

        if (showHeatmaps) {
            for(const auto& tree : pair.second->getTrees()) {
                sf::RectangleShape tb;
                tb.setPosition(tree.getBounds().left, tree.getBounds().top);
                tb.setSize({tree.getBounds().width, tree.getBounds().height});
                tb.setFillColor(sf::Color(255, 165, 0, 80));
                tb.setOutlineColor(sf::Color::Red);
                tb.setOutlineThickness(2.f);
                window.draw(tb);
            }
        }
    }
}

const std::unordered_map<int, std::unique_ptr<Chunk>>& ChunkManager::getActiveChunks() const {
    return chunks;
}

float ChunkManager::getTerrainHeight(float x) const {
    return TerrainGenerator::getTerrainHeight(x, worldSeed);
}

int ChunkManager::getCurrentChunkIndex() const {
    return currentChunkIdx;
}

RegionType ChunkManager::getCurrentRegion(float playerX) const {
    int idx = static_cast<int>(std::floor(playerX / chunkWidth));
    auto it = chunks.find(idx);
    if (it != chunks.end()) {
        return it->second->getRegionType();
    }
    return RegionType::OldGrowth;
}