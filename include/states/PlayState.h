#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "states/GameState.h"
#include "world/WorldManager.h"
#include "world/CameraManager.h"
#include "world/LightingManager.h"
#include "world/WeatherManager.h"
#include "world/ParticleSystem.h"
#include "world/Background.h"
#include "core/WorldClock.h"
#include "core/AudioManager.h"
#include "entities/Ape.h"
#include "core/Profiler.h"
#include "core/DebugOverlay.h"
#include "simulation/SimulationManager.h"
#include "world/NPCManager.h"
#include "world/StructureManager.h" // Phase 5 Additions

class Game;

class PlayState : public GameState {
private:
    Game* game;
    std::unique_ptr<Background> background;
    std::unique_ptr<Ape> playerWrapper;
    std::unique_ptr<WorldManager> worldManager;
    std::unique_ptr<CameraManager> cameraManager;
    std::unique_ptr<LightingManager> lightingManager;
    std::unique_ptr<WeatherManager> weatherManager;
    std::unique_ptr<ParticleSystem> particleSystem;
    std::unique_ptr<AudioManager> audioManager;
    std::unique_ptr<WorldClock> worldClock;
    std::unique_ptr<DebugOverlay> debugOverlay;
    
    std::unique_ptr<sim::SimulationManager> simulationManager;
    std::unique_ptr<NPCManager> npcManager;
    std::unique_ptr<StructureManager> structureManager; // Phase 5 Additions

    ProfilerStats profiler;
    uint32_t activeSeed;

    bool isTransitioning;
    float transitionTimer;
    sf::Vector2f transitionTarget;

    bool f3PressedLastFrame;
    bool f4PressedLastFrame;
    bool f5PressedLastFrame;
    bool f6PressedLastFrame;
    bool f7PressedLastFrame;
    bool f8PressedLastFrame;
    bool f9PressedLastFrame;
    bool f10PressedLastFrame;
    bool f11PressedLastFrame;

public:
    PlayState(Game* game);
    void init() override;
    void processEvents(const sf::Event& event) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
};