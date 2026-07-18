#pragma once
#include "states/GameState.h"
#include "entities/Ape.h"
#include "world/WorldManager.h"
#include "world/CameraManager.h"
#include "world/ParallaxBackground.h"
#include "world/LightingManager.h"
#include "core/WorldClock.h"
#include "core/DebugOverlay.h"
#include <SFML/Graphics.hpp>
#include <memory>

class Game;

class PlayState : public GameState {
private:
    Game* game;
    uint32_t activeSeed;
    std::unique_ptr<Ape> player;
    std::unique_ptr<WorldManager> worldManager;
    std::unique_ptr<CameraManager> cameraManager;
    std::unique_ptr<ParallaxBackground> background;
    std::unique_ptr<LightingManager> lightingManager;
    std::unique_ptr<WorldClock> worldClock;
    std::unique_ptr<DebugOverlay> debugOverlay;

    bool f3PressedLastFrame;
    bool f4PressedLastFrame;
    bool f5PressedLastFrame;
    bool f6PressedLastFrame;

public:
    PlayState(Game* game);
    void init() override;
    void processEvents(const sf::Event& event) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
};