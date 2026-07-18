#include "core/DebugOverlay.h"

DebugOverlay::DebugOverlay() : isVisible(false), fontLoaded(false), updateTimer(0.f), showChunkBorders(false), showRegions(false), showHeatmaps(false), showFoliage(true) {
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

void DebugOverlay::toggle() {
    isVisible = !isVisible;
}

void DebugOverlay::updateInfo(float dt, float fps, int chunkX, float pX, float pY, uint32_t seed, const std::string& biomeName, size_t chunks) {
    if (!isVisible || !fontLoaded) return;
    
    updateTimer += dt;
    if (updateTimer > 0.1f) {
        std::string info = "Ape Dynasty Debug Engine\n";
        info += "FPS: " + std::to_string(static_cast<int>(fps)) + "\n";
        info += "Seed: " + std::to_string(seed) + "\n";
        info += "Player Pos: " + std::to_string(static_cast<int>(pX)) + ", " + std::to_string(static_cast<int>(pY)) + "\n";
        info += "Chunk Index: " + std::to_string(chunkX) + "\n";
        info += "Loaded Chunks: " + std::to_string(chunks) + "\n";
        info += "Biome: " + biomeName + "\n";
        
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

bool DebugOverlay::getVisible() const {
    return isVisible;
}

void DebugOverlay::toggleBorders() { showChunkBorders = !showChunkBorders; }
void DebugOverlay::toggleRegions() { showRegions = !showRegions; }
void DebugOverlay::toggleHeatmaps() { showHeatmaps = !showHeatmaps; }
void DebugOverlay::toggleFoliage() { showFoliage = !showFoliage; }

bool DebugOverlay::getShowBorders() const { return showChunkBorders; }
bool DebugOverlay::getShowRegions() const { return showRegions; }
bool DebugOverlay::getShowHeatmaps() const { return showHeatmaps; }
bool DebugOverlay::getShowFoliage() const { return showFoliage; }