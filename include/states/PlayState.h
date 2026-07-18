#pragma once
#include "states/GameState.h"
#include "entities/Ape.h"
#include "world/WorldManager.h"
#include "world/CameraManager.h"
#include "world/ParallaxBackground.h"
#include "world/LightingManager.h"
#include <SFML/Graphics.hpp>
#include <memory>

class Game;

class PlayState : public GameState {
private:
    Game* game;
    std::unique_ptr<Ape> player;
    std::unique_ptr<WorldManager> worldManager;
    std::unique_ptr<CameraManager> cameraManager;
    std::unique_ptr<ParallaxBackground> background;
    std::unique_ptr<LightingManager> lightingManager;

public:
    PlayState(Game* game);
    void init() override;
    void processEvents(const sf::Event& event) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
};