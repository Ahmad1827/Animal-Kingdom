#include "states/PlayState.h"
#include "core/Game.h"

PlayState::PlayState(Game* game) : game(game) {}

void PlayState::init() {
    player = std::make_unique<Ape>(0.f, 0.f);
    worldManager = std::make_unique<WorldManager>();
    cameraManager = std::make_unique<CameraManager>(sf::Vector2f(1280.f, 720.f));
    background = std::make_unique<ParallaxBackground>();
    lightingManager = std::make_unique<LightingManager>();
}

void PlayState::processEvents(const sf::Event& event) {
}

void PlayState::update(float dt) {
    if (worldManager) {
        worldManager->update(dt, player->getPosition().x);
    }

    if (player) {
        player->update(dt);

        sf::FloatRect playerBounds = player->getBounds();
        sf::FloatRect platformBounds;
        
        if (player->getState() != ApeState::ClimbingTrunk && player->getState() != ApeState::HangingBranch) {
            player->setState(ApeState::Airborne);
        }

        // Rolling Terrain Collision
        float playerCenterX = playerBounds.left + (playerBounds.width / 2.f);
        float groundHeight = worldManager->getTerrainHeight(playerCenterX);

        if (player->getVelocity().y > 0.f && (playerBounds.top + playerBounds.height) >= groundHeight) {
            player->setPosition(player->getPosition().x, groundHeight - playerBounds.height);
            player->setVelocity(player->getVelocity().x, 0.f);
            player->setState(ApeState::Grounded);
            player->setDroppingThrough(false);
        }

        // Trees & Branches Collision
        if (player->getState() == ApeState::Airborne && !player->isDroppingThrough()) {
            if (worldManager->checkOneWayCollision(playerBounds, player->getVelocity(), dt, platformBounds)) {
                player->setPosition(player->getPosition().x, platformBounds.top - playerBounds.height);
                player->setVelocity(player->getVelocity().x, 0.f);
                player->setState(ApeState::Grounded);
            }
        }

        float trunkCenter = 0.f;
        if (worldManager->checkTrunkCollision(playerBounds, trunkCenter)) {
            if ((sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) && 
                player->getState() != ApeState::ClimbingTrunk && 
                player->getState() != ApeState::HangingBranch) {
                player->setState(ApeState::ClimbingTrunk);
                player->setPosition(trunkCenter - (playerBounds.width / 2.f), player->getPosition().y);
                player->setVelocity(0.f, 0.f);
            }
        } else {
            if (player->getState() == ApeState::ClimbingTrunk) {
                player->setState(ApeState::Airborne);
            }
        }

        sf::FloatRect branchBounds;
        if (player->getState() != ApeState::ClimbingTrunk && player->getState() != ApeState::Grounded) {
            if (worldManager->checkHangCollision(playerBounds, branchBounds)) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                    if (player->getVelocity().y >= 0) { 
                        player->setState(ApeState::HangingBranch);
                        player->setPosition(player->getPosition().x, branchBounds.top + branchBounds.height);
                        player->setVelocity(0.f, 0.f);
                    }
                }
            } else if (player->getState() == ApeState::HangingBranch) {
                player->setState(ApeState::Airborne);
            }
        }

        cameraManager->update(dt, player->getPosition(), player->getVelocity());
        background->update(cameraManager->getView().getCenter().x);
        lightingManager->update(dt, cameraManager->getView());
    }
}

void PlayState::draw(sf::RenderWindow& window) {
    if (background) background->draw(window);

    window.setView(cameraManager->getView());
    
    if (worldManager) worldManager->draw(window);
    if (player) player->draw(window);
    
    window.setView(window.getDefaultView());
    if (lightingManager) lightingManager->draw(window);
}