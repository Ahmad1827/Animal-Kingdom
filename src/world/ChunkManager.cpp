#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"
#include <cmath>

ChunkManager::ChunkManager(uint32_t seed) : chunkWidth(3000.f), worldSeed(seed), currentChunkIdx(0) {}

void ChunkManager::update(float playerX) {
    currentChunkIdx = static_cast<int>(std::floor(playerX / chunkWidth));

    // Wider preload distance (4 chunks ahead/behind) to eliminate pop-in
    for (int i = currentChunkIdx - 4; i <= currentChunkIdx + 4; ++i) {
        if (chunks.find(i) == chunks.end()) {
            chunks[i] = std::make_unique<Chunk>(i, chunkWidth, worldSeed);
        }
    }

    // Safely unload far behind
    for (auto it = chunks.begin(); it != chunks.end();) {
        if (std::abs(it->first - currentChunkIdx) > 6) {
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
}

void ChunkManager::draw(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const {
    drawBackground(window, viewBounds, true);
    drawGeometry(window, viewBounds);
}

void ChunkManager::drawBackground(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showFoliage) const {
    sf::FloatRect renderBounds = viewBounds;
    renderBounds.left -= 500.f;
    renderBounds.width += 1000.f;

    for (const auto& pair : chunks) {
        pair.second->drawBackground(window, renderBounds, showFoliage);
    }
}

void ChunkManager::drawGeometry(sf::RenderWindow& window, const sf::FloatRect& viewBounds) const {
    sf::FloatRect renderBounds = viewBounds;
    renderBounds.left -= 500.f;
    renderBounds.width += 1000.f;

    for (const auto& pair : chunks) {
        pair.second->drawGeometry(window, renderBounds);
    }
}

void ChunkManager::drawDebug(sf::RenderWindow& window, const sf::FloatRect& viewBounds, bool showBorders, bool showRegions, bool showHeatmaps) const {
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