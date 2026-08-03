#pragma once
#include <SFML/Graphics.hpp>
#include "states/GameState.h"
#include "core/Game.h"
#include "world/WorldManager.h"
#include "world/Background.h"
#include "entities/Ape.h"
#include "world/CameraManager.h"
#include "world/LightingManager.h"
#include "world/ParticleSystem.h"
#include "core/AudioManager.h"
#include "world/WeatherManager.h"
#include "core/WorldClock.h"
#include "core/DebugOverlay.h"
#include "core/Profiler.h"
#include <memory>

class PlayState : public GameState {
private:
    Game* game;
    uint32_t activeSeed;
    
    std::unique_ptr<Background> background;
    std::unique_ptr<Ape> player;
    std::unique_ptr<WorldManager> worldManager;
    std::unique_ptr<CameraManager> cameraManager;
    std::unique_ptr<LightingManager> lightingManager;
    std::unique_ptr<WeatherManager> weatherManager;
    std::unique_ptr<ParticleSystem> particleSystem;
    std::unique_ptr<AudioManager> audioManager;
    std::unique_ptr<WorldClock> worldClock;
    std::unique_ptr<DebugOverlay> debugOverlay;
    
    ProfilerStats profiler;

    bool f3PressedLastFrame;
    bool f4PressedLastFrame;
    bool f5PressedLastFrame;
    bool f6PressedLastFrame;
    bool f7PressedLastFrame;
    bool f8PressedLastFrame;

    sf::RenderTexture sceneBuffer;
    sf::Shader crtShader;

public:
    PlayState(Game* game);
    void init() override;
    void processEvents(const sf::Event& event) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
};