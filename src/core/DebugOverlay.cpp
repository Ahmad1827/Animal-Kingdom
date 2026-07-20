#include "core/DebugOverlay.h"
#include <sstream>
#include <iomanip>
#include <cmath>

DebugOverlay::DebugOverlay() {
    if (!font.loadFromFile("assets/fonts/arial.ttf")) {}
    text.setFont(font);
    text.setCharacterSize(14);
    text.setFillColor(sf::Color::White);
    text.setOutlineColor(sf::Color::Black);
    text.setOutlineThickness(1.f);
    text.setPosition(10.f, 10.f);
}

void DebugOverlay::toggle() { visible = !visible; }
void DebugOverlay::toggleBorders() { showBorders = !showBorders; }
void DebugOverlay::toggleRegions() { showRegions = !showRegions; }
void DebugOverlay::toggleHeatmaps() { showHeatmaps = !showHeatmaps; }
void DebugOverlay::toggleFoliage() { showFoliage = !showFoliage; }
void DebugOverlay::toggleProfiler() { showProfiler = !showProfiler; }
void DebugOverlay::toggleEngineInternals() { showEngineInternals = !showEngineInternals; }
void DebugOverlay::toggleGenerationDebug() { showGenerationDebug = !showGenerationDebug; }
void DebugOverlay::toggleKinematicsDebug() { showKinematicsDebug = !showKinematicsDebug; }

bool DebugOverlay::getVisible() const { return visible; }
bool DebugOverlay::getShowBorders() const { return showBorders; }
bool DebugOverlay::getShowRegions() const { return showRegions; }
bool DebugOverlay::getShowHeatmaps() const { return showHeatmaps; }
bool DebugOverlay::getShowFoliage() const { return showFoliage; }
bool DebugOverlay::getShowGenerationDebug() const { return showGenerationDebug; }
bool DebugOverlay::getShowKinematicsDebug() const { return showKinematicsDebug; }

void DebugOverlay::updateInfo(float dt, int chunkIdx, float px, float py, uint32_t seed, const std::string& region, const ProfilerStats& profiler) {
    std::ostringstream ss;
    
    if (showKinematicsDebug) {
        ss << std::fixed << std::setprecision(2);
        ss << "--- KINEMATICS & PHYSICS DEBUG (F11) ---\n";
        ss << "Phys Pos:   (" << profiler.playerPos.x << ", " << profiler.playerPos.y << ")\n";
        ss << "Rend Pos:   (" << profiler.playerPos.x << ", " << std::floor(profiler.playerPos.y) << ")\n"; // Rendering snaps to floor
        ss << "Gnd Height: " << profiler.groundHeight << "\n";
        ss << "Distance:   " << (profiler.groundHeight - profiler.playerPos.y) << "\n";
        ss << "Vel Y:      " << profiler.verticalVelocity << "\n";
        ss << "Grounded:   " << (profiler.isGrounded ? "TRUE" : "FALSE") << "\n\n";
        ss << "Cam Pos:    (" << profiler.cameraPos.x << ", " << profiler.cameraPos.y << ")\n";
        ss << "Cam Target: (" << profiler.cameraTarget.x << ", " << profiler.cameraTarget.y << ")\n";
    }
    else if (showProfiler) {
        ss << std::fixed << std::setprecision(2);
        ss << "--- PERFORMANCE PROFILER (F10) ---\n";
        ss << "FPS: " << profiler.fps << " | Frame: " << profiler.frameTime << " ms\n";
        ss << "Physics: " << profiler.physicsTime * 1000.f << " ms\n";
        ss << "Camera: " << profiler.cameraTime * 1000.f << " ms\n";
        ss << "Particles: " << profiler.particleTime * 1000.f << " ms\n";
        ss << "Draw Calls: " << profiler.drawCalls << "\n\n";
        ss << "--- WORLD STREAMING ---\n";
        ss << "Chunks [Vis/Load]: " << profiler.visibleChunks << " / " << profiler.loadedChunks << "\n";
        ss << "Trees [Vis/Load]: " << profiler.visibleTrees << " / " << profiler.loadedTrees << "\n";
        ss << "Decors [Vis/Load]: " << profiler.visibleDecorations << " / " << profiler.loadedDecorations << "\n";
        ss << "Async Queue: " << profiler.chunksGenerating << " | Cached: " << profiler.chunksCached << "\n";
        ss << "Total Gen Time: " << profiler.lastChunkGenTime * 1000.f << " ms\n";
    } else {
        ss << "FPS: " << static_cast<int>(profiler.fps) << "\n";
        ss << "Pos: " << static_cast<int>(px) << ", " << static_cast<int>(py) << "\n";
        ss << "Chunk: " << chunkIdx << " | Region: " << region << "\n";
        ss << "F10: Profiler | F11: Physics Debug\n";
    }

    text.setString(ss.str());
}

void DebugOverlay::draw(sf::RenderWindow& window) const {
    if (visible) {
        sf::View currentView = window.getView();
        window.setView(window.getDefaultView());
        window.draw(text);
        window.setView(currentView);
    }
}