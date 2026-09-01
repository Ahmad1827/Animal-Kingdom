#include "core/Game.h"
#include "states/PlayState.h"

Game::Game() : window(sf::VideoMode(1280, 720), "Ape Dynasty") {
    assetManager.loadTexture("bg1", "assets/sprites/BG1.png", true);
    assetManager.loadTexture("bg2", "assets/sprites/BG2.png", true);
    assetManager.loadTexture("bg3", "assets/sprites/BG3.png", true);
    assetManager.loadTexture("layer1_sky", "assets/sprites/LAYER1FIX.png");
    assetManager.loadTexture("layer2_mountains", "assets/sprites/LAYER2FIX.png");
    assetManager.loadTexture("layer3_hills", "assets/sprites/LAYER3FIX.png");
    assetManager.loadTexture("tileset", "assets/sprites/Tileset.png", true);
    assetManager.loadTexture("decors", "assets/sprites/Decors.png", false);
    assetManager.loadTexture("playerTex", "assets/sprites/ApeSpriteSheet.png");

    assetManager.loadTexture("jungle_ground", "assets/sprites/assetsfixed3.png", false);

    assetManager.loadTexture("tree1", "assets/sprites/TREEFIXED_1.png", false);
    assetManager.loadTexture("tree2", "assets/sprites/TREEFIXED_2.png", false);
    assetManager.loadTexture("tree3", "assets/sprites/TREEFIXED_3.png", false);
    assetManager.loadTexture("tree4", "assets/sprites/TREEFIXED_4.png", false);
    assetManager.loadTexture("tree5", "assets/sprites/TREEFIXED_5.png", false);

    assetManager.loadTexture("village_assets", "assets/sprites/villageassets3.png", false);

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
        window.setMouseCursorVisible(false);
        window.clear(sf::Color(20, 20, 25));
        stateMachine.draw(window);
        window.display();
    }
}