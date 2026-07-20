#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"
#include "core/DebugOverlay.h"
#include <cmath>
#include <chrono>

ChunkManager::ChunkManager(uint32_t seed, sf::Texture& decorTex) : chunkWidth(2000.f), chunkHeight(2000.f), worldSeed(seed), globalDecorTex(decorTex), currentChunkIdx(0) {}

uint64_t ChunkManager::getChunkKey(int x, int y) const {
    return ((uint64_t)(uint32_t)x << 32) | (uint32_t)y;
}

void ChunkManager::update(const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, ProfilerStats& profiler) {
    sf::Clock loadClock;

    int minLoadX = static_cast<int>(std::floor(preloadBounds.left / chunkWidth));
    int maxLoadX = static_cast<int>(std::floor((preloadBounds.left + preloadBounds.width) / chunkWidth));
    int minLoadY = static_cast<int>(std::floor(preloadBounds.top / chunkHeight));
    int maxLoadY = static_cast<int>(std::floor((preloadBounds.top + preloadBounds.height) / chunkHeight));

    for (auto it = pendingChunks.begin(); it != pendingChunks.end();) {
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            insertionQueue.push(it->second.get());
            it = pendingChunks.erase(it);
        } else {
            ++it;
        }
    }

    int insertionsThisFrame = 0;
    while (!insertionQueue.empty() && insertionsThisFrame < 2) {
        auto newChunk = std::move(insertionQueue.front());
        insertionQueue.pop();
        
        profiler.lastTerrainGenTime = newChunk->getTerrainGenTime();
        profiler.lastTreeGenTime = newChunk->getTreeGenTime();
        profiler.lastChunkGenTime = newChunk->getTotalGenTime();
        
        uint64_t key = getChunkKey(newChunk->getPos().x, newChunk->getPos().y);
        chunks[key] = std::move(newChunk);
        insertionsThisFrame++;
    }

    for (int x = minLoadX; x <= maxLoadX; ++x) {
        for (int y = minLoadY; y <= maxLoadY; ++y) {
            uint64_t key = getChunkKey(x, y);
            if (chunks.find(key) == chunks.end() && pendingChunks.find(key) == pendingChunks.end()) {
                if (cachedChunks.find(key) != cachedChunks.end()) {
                    chunks[key] = std::move(cachedChunks[key]);
                    cachedChunks.erase(key);
                } else {
                    uint32_t seed = worldSeed;
                    float cw = chunkWidth;
                    float ch = chunkHeight;
                    ChunkPos pos{x, y};
                    sf::Texture* texPtr = &globalDecorTex;
                    pendingChunks[key] = std::async(std::launch::async, [pos, cw, ch, seed, texPtr]() {
                        return std::make_unique<Chunk>(pos, cw, ch, seed, *texPtr);
                    });
                }
            }
        }
    }

    for (auto it = chunks.begin(); it != chunks.end();) {
        sf::FloatRect cb = it->second->getBounds();
        if (cb.left + cb.width < unloadBounds.left || cb.left > unloadBounds.left + unloadBounds.width ||
            cb.top + cb.height < unloadBounds.top || cb.top > unloadBounds.top + unloadBounds.height) {
            cachedChunks[it->first] = std::move(it->second);
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
    
    currentChunkIdx = static_cast<int>(std::floor((preloadBounds.left + (preloadBounds.width / 2.f)) / chunkWidth));
    
    profiler.chunksLoaded = chunks.size();
    profiler.chunksGenerating = pendingChunks.size();
    profiler.chunksQueuedForInsertion = insertionQueue.size();
    profiler.chunksCached = cachedChunks.size();
    profiler.asyncLoadTime = loadClock.getElapsedTime().asSeconds();

    for (const auto& pair : chunks) {
        profiler.loadedTrees += pair.second->getTrees().size();
        profiler.loadedDecorations += pair.second->getDecorations().size();
    }
}

void ChunkManager::updateSway(float globalTime, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector) {
    int minX = getChunkXAt(viewBounds.left) - 1;
    int maxX = getChunkXAt(viewBounds.left + viewBounds.width) + 1;
    int minY = getChunkYAt(viewBounds.top) - 1;
    int maxY = getChunkYAt(viewBounds.top + viewBounds.height) + 1;

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            Chunk* chunk = getChunk(x, y);
            if (chunk) chunk->updateSway(globalTime, viewBounds, windVector);
        }
    }
}

int ChunkManager::getChunkXAt(float x) const { return static_cast<int>(std::floor(x / chunkWidth)); }
int ChunkManager::getChunkYAt(float y) const { return static_cast<int>(std::floor(y / chunkHeight)); }

Chunk* ChunkManager::getChunk(int cx, int cy) const {
    auto it = chunks.find(getChunkKey(cx, cy));
    if (it != chunks.end()) return it->second.get();
    return nullptr;
}

const std::unordered_map<uint64_t, std::unique_ptr<Chunk>>& ChunkManager::getActiveChunks() const { return chunks; }

void ChunkManager::drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage, ProfilerStats& profiler, sf::Texture& tileset) const {
    sf::FloatRect renderBounds = viewBounds;
    renderBounds.left -= 3000.f; renderBounds.width += 6000.f;
    renderBounds.top -= 3000.f; renderBounds.height += 6000.f;
    
    int minX = getChunkXAt(renderBounds.left);
    int maxX = getChunkXAt(renderBounds.left + renderBounds.width);
    int minY = getChunkYAt(renderBounds.top);
    int maxY = getChunkYAt(renderBounds.top + renderBounds.height);

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            Chunk* chunk = getChunk(x, y);
            if (chunk) {
                chunk->drawBackground(window, renderBounds, showFoliage, profiler, tileset);
                if (chunk->getBounds().intersects(viewBounds)) {
                    profiler.visibleChunks++;
                }
            }
        }
    }
}

void ChunkManager::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds, ProfilerStats& profiler) const {
    sf::FloatRect renderBounds = viewBounds;
    renderBounds.left -= 3000.f; renderBounds.width += 6000.f;
    renderBounds.top -= 3000.f; renderBounds.height += 6000.f;
    
    int minX = getChunkXAt(renderBounds.left);
    int maxX = getChunkXAt(renderBounds.left + renderBounds.width);
    int minY = getChunkYAt(renderBounds.top);
    int maxY = getChunkYAt(renderBounds.top + renderBounds.height);

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            Chunk* chunk = getChunk(x, y);
            if (chunk) chunk->drawGeometry(window, renderBounds, profiler);
        }
    }
}

void ChunkManager::drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, const sf::FloatRect& preloadBounds, const sf::FloatRect& unloadBounds, DebugOverlay* debugOverlay) const {
    if (!debugOverlay) return;
    
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
        sf::FloatRect cb = pair.second->getBounds();
        float startX = cb.left;
        float startY = cb.top;
        
        if (debugOverlay->getShowBorders()) {
            sf::RectangleShape border({chunkWidth, chunkHeight});
            border.setPosition(startX, startY);
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineColor(sf::Color(255, 0, 0, 150));
            border.setOutlineThickness(2.f);
            window.draw(border);
        }
        
        if (debugOverlay->getShowRegions()) {
            BiomeProperties props = Biome::getProperties(pair.second->getRegionType());
            sf::RectangleShape regionOverlay({chunkWidth, chunkHeight});
            regionOverlay.setPosition(startX, startY);
            regionOverlay.setFillColor(props.debugColor);
            window.draw(regionOverlay);
        }

        if (debugOverlay->getShowHeatmaps()) {
            for (const auto& tree : pair.second->getTrees()) {
                sf::RectangleShape tb;
                sf::FloatRect trBounds = tree.getBounds();
                tb.setPosition(trBounds.left, trBounds.top);
                tb.setSize({trBounds.width, trBounds.height});
                tb.setFillColor(sf::Color(255, 165, 0, 40));
                tb.setOutlineColor(sf::Color::Red);
                tb.setOutlineThickness(1.f);
                window.draw(tb);
            }
        }
        
        if (debugOverlay->getShowGenerationDebug()) {
            BiomeProperties props = Biome::getProperties(pair.second->getRegionType());
            for (const auto& tree : pair.second->getTrees()) {
                sf::FloatRect trBounds = tree.getTrunkBounds();
                
                sf::CircleShape anchor(4.f);
                anchor.setFillColor(sf::Color::Yellow);
                anchor.setOrigin(4.f, 4.f);
                anchor.setPosition(trBounds.left + trBounds.width/2.f, trBounds.top + trBounds.height);
                window.draw(anchor);
                
                sf::RectangleShape exclusion({props.minTreeSpacing, 10.f});
                exclusion.setPosition(trBounds.left + trBounds.width/2.f - props.minTreeSpacing/2.f, trBounds.top + trBounds.height);
                exclusion.setFillColor(sf::Color(255, 0, 255, 100));
                window.draw(exclusion);
            }

            for (const auto& decor : pair.second->getDecorations()) {
                sf::FloatRect cBounds = decor.getBounds();
                sf::RectangleShape cb({cBounds.width, cBounds.height});
                cb.setPosition(cBounds.left, cBounds.top);
                cb.setFillColor(sf::Color::Transparent);
                cb.setOutlineColor(sf::Color::Cyan);
                cb.setOutlineThickness(1.f);
                window.draw(cb);

                sf::CircleShape dAnchor(2.f);
                dAnchor.setFillColor(sf::Color::Red);
                dAnchor.setOrigin(2.f, 2.f);
                dAnchor.setPosition(cBounds.left + cBounds.width / 2.f, cBounds.top + cBounds.height);
                window.draw(dAnchor);
            }
        }
    }
}

float ChunkManager::getTerrainHeight(float x) const { return TerrainGenerator::getTerrainHeight(x, worldSeed); }
int ChunkManager::getCurrentChunkIndex() const { return currentChunkIdx; }
RegionType ChunkManager::getCurrentRegion(float playerX) const { return Biome::determineRegion(getChunkXAt(playerX), worldSeed); }