#pragma once
#include <SFML/Graphics.hpp>
#include "core/AssetManager.h"
#include "core/StateMachine.h"

class Game {
private:
    sf::RenderWindow window;
    AssetManager assetManager;
    StateMachine stateMachine;
    sf::Clock clock;

public:
    Game();
    void run();
    AssetManager& getAssetManager() { return assetManager; }
};