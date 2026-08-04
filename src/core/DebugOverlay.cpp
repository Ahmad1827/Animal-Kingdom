#include "core/DebugOverlay.h"
#include <sstream>
#include <iomanip>
#include <cmath>

DebugOverlay::DebugOverlay() : isVisible(false), showBorders(false), showRegions(false), showHeatmaps(false), showFoliage(false), showGenerationDebug(false), showProfiler(false), showEngineInternals(false), showKinematicsDebug(false) {
    if (!font.loadFromFile("assets/fonts/arial.ttf")) {}
    
    debugText.setFont(font);
    debugText.setCharacterSize(14);
    debugText.setFillColor(sf::Color::White);
    debugText.setOutlineColor(sf::Color::Black);
    debugText.setOutlineThickness(1.f);
    debugText.setPosition(10.f, 10.f);

    simText.setFont(font);
    simText.setCharacterSize(14);
    simText.setFillColor(sf::Color::Yellow);
    simText.setOutlineColor(sf::Color::Black);
    simText.setOutlineThickness(1.f);
    simText.setPosition(10.f, 400.f);

    dynText.setFont(font);
    dynText.setCharacterSize(14);
    dynText.setFillColor(sf::Color::Cyan);
    dynText.setOutlineColor(sf::Color::Black);
    dynText.setOutlineThickness(1.f);
    dynText.setPosition(10.f, 200.f);
}

void DebugOverlay::toggle() { isVisible = !isVisible; }
void DebugOverlay::toggleBorders() { showBorders = !showBorders; }
void DebugOverlay::toggleRegions() { showRegions = !showRegions; }
void DebugOverlay::toggleHeatmaps() { showHeatmaps = !showHeatmaps; }
void DebugOverlay::toggleFoliage() { showFoliage = !showFoliage; }
void DebugOverlay::toggleProfiler() { showProfiler = !showProfiler; }
void DebugOverlay::toggleEngineInternals() { showEngineInternals = !showEngineInternals; }
void DebugOverlay::toggleGenerationDebug() { showGenerationDebug = !showGenerationDebug; }
void DebugOverlay::toggleKinematicsDebug() { showKinematicsDebug = !showKinematicsDebug; }

bool DebugOverlay::getVisible() const { return isVisible; }
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
        ss << "Rend Pos:   (" << profiler.playerPos.x << ", " << std::floor(profiler.playerPos.y) << ")\n";
        ss << "Gnd Height: " << profiler.groundHeight << "\n";
        ss << "Distance:   " << (profiler.groundHeight - profiler.playerPos.y) << "\n";
        ss << "Vel Y:      " << profiler.verticalVelocity << "\n";
        ss << "Grounded:   " << (profiler.isGrounded ? "TRUE" : "FALSE") << "\n\n";
        ss << "Cam Pos:    (" << profiler.cameraPos.x << ", " << profiler.cameraPos.y << ")\n";
        ss << "Cam Target: (" << profiler.cameraTarget.x << ", " << profiler.cameraTarget.y << ")\n";
        ss << "--- ANIMATION STATE ---\n";
        std::string stateStr;
        switch(profiler.playerStateInt) {
            case 0: stateStr = "Airborne"; break;
            case 1: stateStr = "Grounded"; break;
            case 2: stateStr = "ClimbingTrunk"; break;
            case 3: stateStr = "ClimbingVine"; break;
            case 4: stateStr = "HangingBranch"; break;
            default: stateStr = "Unknown"; break;
        }

        ss << "--- STRICT ANIMATION TRACE ---\n";
        ss << "State:      " << stateStr << "\n";
        ss << "Animation:  " << profiler.animName << " (Frame " << profiler.animFrame << ")\n";
        ss << "Time / dt:  " << profiler.animTime << " / " << profiler.currentDt << "\n";
        ss << "Physics Pos:(" << profiler.playerPos.x << ", " << profiler.playerPos.y << ")\n";
        ss << "Velocity:   (" << profiler.verticalVelocity << " Y)\n";
        ss << "Scale:      (" << profiler.spriteScale.x << ", " << profiler.spriteScale.y << ")\n";
        ss << "TexRect:    [" << profiler.animRect.left << ", " << profiler.animRect.top << ", " << profiler.animRect.width << ", " << profiler.animRect.height << "]\n";
        ss << "Origin:     (" << profiler.spriteOrigin.x << ", " << profiler.spriteOrigin.y << ")\n";
        ss << "Offset:     (" << profiler.animOffset.x << ", " << profiler.animOffset.y << ")\n";
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

    debugText.setString(ss.str());
}

void DebugOverlay::updateSimStats(int simApes, int loadedNPCs, int loadedChunks, uint64_t simTick, int hour, int min, int day, int season, int year) {
    if (!isVisible) return;

    std::string info = "--- DYNASTY SIMULATION ---\n";
    info += "Simulated Apes: " + std::to_string(simApes) + "\n";
    info += "Loaded NPCs: " + std::to_string(loadedNPCs) + "\n";
    info += "Loaded Chunks: " + std::to_string(loadedChunks) + "\n";
    info += "Sim Tick: " + std::to_string(simTick) + "\n";
    
    char timeStr[64];
    std::snprintf(timeStr, sizeof(timeStr), "Time: %02d:%02d | Day: %d | Season: %d | Year: %d", hour, min, day, season, year);
    info += std::string(timeStr) + "\n";

    simText.setString(info);
}

void DebugOverlay::updateDynastyStats(const std::string& name, float age, float health, const std::string& dynName, uint64_t id, uint64_t heirId, int livingCount) {
    if (!isVisible) return;

    std::string info = "--- CONTROLLED ENTITY ---\n";
    info += "Name: " + name + "\n";
    info += "Age: " + std::to_string(static_cast<int>(age)) + " | Health: " + std::to_string(static_cast<int>(health)) + "\n";
    info += "Dynasty: " + dynName + "\n";
    info += "Entity ID: " + std::to_string(id) + "\n";
    info += "Current Heir ID: " + std::to_string(heirId) + "\n";
    info += "Living Members: " + std::to_string(livingCount) + "\n";

    dynText.setString(info);
}

void DebugOverlay::draw(sf::RenderTarget& target) const {
    if (isVisible) {
        sf::View currentView = target.getView();
        target.setView(target.getDefaultView());
        target.draw(debugText);
        target.draw(simText);
        target.draw(dynText);
        target.setView(currentView);
    }
}