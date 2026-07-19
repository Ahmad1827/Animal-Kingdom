#include "core/Game.h"
#include "states/PlayState.h"

Game::Game() : window(sf::VideoMode(1280, 720), "Ape Dynasty") {
    assetManager.loadTexture("bg1", "assets/sprites/BG1.png", true);
    assetManager.loadTexture("bg2", "assets/sprites/BG2.png", true);
    assetManager.loadTexture("bg3", "assets/sprites/BG3.png", true);
    assetManager.loadTexture("tileset", "assets/sprites/Tileset.png", true);
    assetManager.loadTexture("decors", "assets/sprites/Decors.png", false);
    window.setFramerateLimit(60);
    stateMachine.addState(std::make_unique<PlayState>(this));
}

void Game::run() {
    while (window.isOpen()) {
        sf::Time dt = clock.restart();
        
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            stateMachine.processEvents(event);
        }

        stateMachine.update(dt.asSeconds());

        window.clear(sf::Color(20, 20, 25));
        stateMachine.draw(window);
        window.display();
    }
}