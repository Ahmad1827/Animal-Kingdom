#include "core/DebugOverlay.h"
#include <cmath>

DebugOverlay::DebugOverlay() : isVisible(false), fontLoaded(false), updateTimer(0.f), showChunkBorders(false), showRegions(false), showHeatmaps(false), showFoliage(true), showProfiler(false), showEngineInternals(false), showGenerationDebug(false) {
    if (font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        fontLoaded = true;
    } else if (font.loadFromFile("assets/fonts/arial.ttf")) {
        fontLoaded = true;
    }
    
    if (fontLoaded) {
        text.setFont(font);
        text.setCharacterSize(14);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.f);
        text.setPosition(10.f, 10.f);
    }
}

void DebugOverlay::toggle() { isVisible = !isVisible; }
void DebugOverlay::toggleBorders() { showChunkBorders = !showChunkBorders; }
void DebugOverlay::toggleRegions() { showRegions = !showRegions; }
void DebugOverlay::toggleHeatmaps() { showHeatmaps = !showHeatmaps; }
void DebugOverlay::toggleFoliage() { showFoliage = !showFoliage; }
void DebugOverlay::toggleProfiler() { showProfiler = !showProfiler; }
void DebugOverlay::toggleEngineInternals() { showEngineInternals = !showEngineInternals; }
void DebugOverlay::toggleGenerationDebug() { showGenerationDebug = !showGenerationDebug; }
bool DebugOverlay::getShowGenerationDebug() const { return showGenerationDebug; }

void DebugOverlay::updateInfo(float dt, int chunkX, float pX, float pY, uint32_t seed, const std::string& biomeName, const ProfilerStats& profiler) {
    if (!isVisible || !fontLoaded) return;
    
    updateTimer += dt;
    if (updateTimer > 0.1f) {
        std::string info = "Ape Dynasty Debug Engine (F3)\n";
        info += "Player Pos: " + std::to_string(static_cast<int>(pX)) + ", " + std::to_string(static_cast<int>(pY)) + "\n";
        info += "Chunk Index: " + std::to_string(chunkX) + "\n";
        info += "Biome: " + biomeName + "\n";
        info += "Seed: " + std::to_string(seed) + "\n\n";

        if (showProfiler) {
            info += "--- PROFILER (F8) ---\n";
            info += "FPS: " + std::to_string(static_cast<int>(profiler.fps)) + "\n";
            info += "Frame Time: " + std::to_string(static_cast<int>(profiler.frameTime)) + " ms\n";
            info += "Update Time: " + std::to_string(static_cast<int>(profiler.updateTime * 1000.f)) + " ms\n";
            info += "Render Time: " + std::to_string(static_cast<int>(profiler.renderTime * 1000.f)) + " ms\n";
            info += "Collision Time: " + std::to_string(static_cast<int>(profiler.collisionTime * 1000.f)) + " ms\n";
            info += "Async Dispatch: " + std::to_string(static_cast<int>(profiler.asyncLoadTime * 1000.f)) + " ms\n";
            info += "Particle Update: " + std::to_string(static_cast<int>(profiler.particleTime * 1000.f)) + " ms\n";
            info += "Objects Rendered: " + std::to_string(profiler.objectsRendered) + "\n";
            info += "Objects Culled: " + std::to_string(profiler.objectsCulled) + "\n";
        }
        
        if (showEngineInternals) {
            info += "--- ENGINE INTERNALS (F9) ---\n";
            info += "Chunk Gen Time: " + std::to_string(static_cast<int>(profiler.lastChunkGenTime * 1000.f)) + " ms\n";
            info += "Terrain Gen: " + std::to_string(static_cast<int>(profiler.lastTerrainGenTime * 1000.f)) + " ms\n";
            info += "Tree Gen: " + std::to_string(static_cast<int>(profiler.lastTreeGenTime * 1000.f)) + " ms\n";
            info += "Loaded Chunks (2D): " + std::to_string(profiler.chunksLoaded) + "\n";
            info += "Generating (Threads): " + std::to_string(profiler.chunksGenerating) + "\n";
            info += "Pending Insertion Queue: " + std::to_string(profiler.chunksQueuedForInsertion) + "\n";
            info += "Cached Chunks: " + std::to_string(profiler.chunksCached) + "\n";
            info += "Cam Rect Center: " + std::to_string(static_cast<int>(pX)) + ", " + std::to_string(static_cast<int>(pY)) + "\n";
            info += "Vertical Grid Y: " + std::to_string(static_cast<int>(std::floor(pY / 2000.f))) + "\n";
            info += "\n--- ATMOSPHERE ---\n";
            info += "Weather: " + profiler.weatherString + "\n";
            info += "Season: " + profiler.seasonString + "\n";
            info += "Particles Active: " + std::to_string(profiler.particleCount) + "\n";
            info += "Time (0-1): " + std::to_string(profiler.timeOfDay) + "\n";
        }

        text.setString(info);
        updateTimer = 0.f;
    }
}

void DebugOverlay::draw(sf::RenderWindow& window) const {
    if (isVisible && fontLoaded) {
        sf::View current = window.getView();
        window.setView(window.getDefaultView());
        window.draw(text);
        window.setView(current);
    }
}

bool DebugOverlay::getVisible() const { return isVisible; }
bool DebugOverlay::getShowBorders() const { return showChunkBorders; }
bool DebugOverlay::getShowRegions() const { return showRegions; }
bool DebugOverlay::getShowHeatmaps() const { return showHeatmaps; }
bool DebugOverlay::getShowFoliage() const { return showFoliage; }