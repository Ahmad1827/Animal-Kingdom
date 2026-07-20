#include "states/PlayState.h"
#include "core/Game.h"
#include "world/Biome.h"
#include <cstdlib>
#include <ctime>
#include <cmath>

PlayState::PlayState(Game* game) : game(game), f3PressedLastFrame(false), f4PressedLastFrame(false), f5PressedLastFrame(false), f6PressedLastFrame(false), f7PressedLastFrame(false), f8PressedLastFrame(false) {}

void PlayState::init() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    activeSeed = std::rand();
    background = std::make_unique<Background>(game->getAssetManager());
    player = std::make_unique<Ape>(0.f, 0.f);
    worldManager = std::make_unique<WorldManager>(activeSeed, game->getAssetManager().getTexture("decors"));
    cameraManager = std::make_unique<CameraManager>(sf::Vector2f(1280.f, 720.f));
    lightingManager = std::make_unique<LightingManager>();
    weatherManager = std::make_unique<WeatherManager>();
    particleSystem = std::make_unique<ParticleSystem>();
    audioManager = std::make_unique<AudioManager>();
    worldClock = std::make_unique<WorldClock>();
    debugOverlay = std::make_unique<DebugOverlay>();
    
    worldClock->setMultiplier(50.f);
}

void PlayState::processEvents(const sf::Event& event) {
}

void PlayState::update(float dt) {
    profiler.resetPerFrame();
    profiler.fps = (dt > 0.f) ? 1.f / dt : 0.f;
    profiler.frameTime = dt * 1000.f;
    
    sf::Clock updateClock;
    worldClock->update(dt);

    bool f3Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F3);
    if (f3Pressed && !f3PressedLastFrame) debugOverlay->toggle();
    f3PressedLastFrame = f3Pressed;

    bool f4Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F4);
    if (f4Pressed && !f4PressedLastFrame) debugOverlay->toggleBorders();
    f4PressedLastFrame = f4Pressed;

    bool f5Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F5);
    if (f5Pressed && !f5PressedLastFrame) debugOverlay->toggleRegions();
    f5PressedLastFrame = f5Pressed;

    bool f6Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F6);
    if (f6Pressed && !f6PressedLastFrame) debugOverlay->toggleHeatmaps();
    f6PressedLastFrame = f6Pressed;

    bool f7Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F7);
    if (f7Pressed && !f7PressedLastFrame) debugOverlay->toggleFoliage();
    f7PressedLastFrame = f7Pressed;

    bool f8Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F8);
    if (f8Pressed && !f8PressedLastFrame) debugOverlay->toggleProfiler();
    f8PressedLastFrame = f8Pressed;

    static bool f9PressedLastFrame = false;
    bool f9Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F9);
    if (f9Pressed && !f9PressedLastFrame) debugOverlay->toggleEngineInternals();
    f9PressedLastFrame = f9Pressed;

    static bool f10PressedLastFrame = false;
    bool f10Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F10);
    if (f10Pressed && !f10PressedLastFrame) debugOverlay->toggleGenerationDebug(); // Or toggleProfiler based on your mapping
    f10PressedLastFrame = f10Pressed;

    // F11 Toggle Integration
    static bool f11PressedLastFrame = false;
    bool f11Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F11);
    if (f11Pressed && !f11PressedLastFrame) debugOverlay->toggleKinematicsDebug();
    f11PressedLastFrame = f11Pressed;

    background->update(
    cameraManager->getViewBounds().left + cameraManager->getViewBounds().width / 2.f,
    cameraManager->getViewBounds().top + cameraManager->getViewBounds().height / 2.f,
    cameraManager->getView().getSize(),
    dt
    );
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) cameraManager->setZoom(0.5f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) cameraManager->setZoom(2.0f);
    else cameraManager->setZoom(1.35f);

    if (player) {
        player->update(dt);

        sf::Clock physicsClock;
        sf::FloatRect playerBounds = player->getBounds();
        sf::FloatRect platformBounds;
        
        // 1. CAPTURE STATE BEFORE MODIFICATION
        bool wasGrounded = (player->getState() == ApeState::Grounded);

        // 2. FORCED RESET
        if (player->getState() != ApeState::ClimbingTrunk && player->getState() != ApeState::HangingBranch && player->getState() != ApeState::ClimbingVine) {
            player->setState(ApeState::Airborne);
        }

        float playerCenterX = playerBounds.left + (playerBounds.width / 2.f);
        float groundHeight = worldManager->getTerrainHeight(playerCenterX);

        float bottomY = playerBounds.top + playerBounds.height;
        float distanceToGround = groundHeight - bottomY;

        // 3. SLOPE STICKINESS RESOLUTION
        if (player->getVelocity().y >= 0.f && bottomY >= groundHeight) {
            // Direct collision (Uphill or falling onto flat ground)
            player->setPosition(player->getPosition().x, groundHeight - playerBounds.height);
            player->setVelocity(player->getVelocity().x, 0.f);
            player->setState(ApeState::Grounded);
            player->setDroppingThrough(false);
        } 
        else if (wasGrounded && player->getVelocity().y >= 0.f && distanceToGround > 0.f && distanceToGround < 25.f) {
            // Downhill Stickiness: Snap down to the terrain if it falls out from under us
            player->setPosition(player->getPosition().x, groundHeight - playerBounds.height);
            player->setVelocity(player->getVelocity().x, 0.f);
            player->setState(ApeState::Grounded);
            player->setDroppingThrough(false);
        }

        // 4. ONE WAY PLATFORMS
        if (player->getState() == ApeState::Airborne && !player->isDroppingThrough()) {
            if (worldManager->checkOneWayCollision(playerBounds, player->getVelocity(), dt, platformBounds)) {
                player->setPosition(player->getPosition().x, platformBounds.top - playerBounds.height);
                player->setVelocity(player->getVelocity().x, 0.f);
                player->setState(ApeState::Grounded);
            }
        }

        // 5. PHYSICAL IMPACTS
        if (!wasGrounded && player->getState() == ApeState::Grounded && std::abs(player->getVelocity().y) > 300.f) {
            particleSystem->spawnImpactLeaves(player->getPosition() + sf::Vector2f(playerBounds.width/2.f, playerBounds.height), 8);
        }
        
        if (std::abs(player->getVelocity().x) > 10.f) {
            worldManager->disturbEnvironment(playerBounds, player->getVelocity().x);
        }

        float trunkCenter = 0.f;
        float vineCenter = 0.f;
        
        if (worldManager->checkTrunkCollision(playerBounds, trunkCenter)) {
            if ((sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) && 
                player->getState() != ApeState::ClimbingTrunk && 
                player->getState() != ApeState::HangingBranch) {
                player->setState(ApeState::ClimbingTrunk);
                player->setPosition(trunkCenter - (playerBounds.width / 2.f), player->getPosition().y);
                player->setVelocity(0.f, 0.f);
            }
        } else if (worldManager->checkVineCollision(playerBounds, vineCenter)) {
            if ((sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) && 
                player->getState() != ApeState::ClimbingVine) {
                player->setState(ApeState::ClimbingVine);
                player->setPosition(vineCenter - (playerBounds.width / 2.f), player->getPosition().y);
                player->setVelocity(0.f, 0.f);
            }
        } else {
            if (player->getState() == ApeState::ClimbingTrunk || player->getState() == ApeState::ClimbingVine) {
                player->setState(ApeState::Airborne);
            }
        }

        sf::FloatRect branchBounds;
        if (player->getState() != ApeState::ClimbingTrunk && player->getState() != ApeState::ClimbingVine && player->getState() != ApeState::Grounded) {
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
        
        profiler.physicsTime = physicsClock.getElapsedTime().asSeconds();

        sf::Clock cameraClock;
        cameraManager->update(dt, player->getPosition(), player->getVelocity(), player->getState());
        profiler.cameraTime = cameraClock.getElapsedTime().asSeconds();

        sf::FloatRect preloadBounds = cameraManager->getPreloadBounds(player->getVelocity());
        sf::FloatRect unloadBounds = cameraManager->getUnloadBounds();

        if (worldManager) {
            worldManager->update(dt, preloadBounds, unloadBounds, profiler);
        }

        sf::Clock pClock;
        weatherManager->update(dt);
        worldManager->updateSway(dt, cameraManager->getViewBounds(), weatherManager->getWindVector());
        particleSystem->update(dt, cameraManager->getViewBounds(), weatherManager->getWindVector(), weatherManager->getRainIntensity(), worldClock->getTimeOfDay());
        profiler.particleTime = pClock.getElapsedTime().asSeconds();
        
        audioManager->update(dt, weatherManager->getWindIntensity(), weatherManager->getRainIntensity(), worldClock->getTimeOfDay());
        lightingManager->update(dt, cameraManager->getView(), worldClock->getTimeOfDay(), weatherManager->getFogDensity());

        profiler.playerPos = player->getPosition();
        profiler.cameraPos = cameraManager->getView().getCenter();
        profiler.cameraTarget = cameraManager->getIdealPosition();
        profiler.groundHeight = groundHeight;
        profiler.verticalVelocity = player->getVelocity().y;
        profiler.isGrounded = (player->getState() == ApeState::Grounded);

        if (debugOverlay->getVisible()) {
            ChunkManager* cm = worldManager->getChunkManager();
            std::string regionName = Biome::getProperties(cm->getCurrentRegion(player->getPosition().x)).name;
            debugOverlay->updateInfo(dt, cm->getCurrentChunkIndex(), player->getPosition().x, player->getPosition().y, 
                                     activeSeed, regionName, profiler);
        }
    }
    
    profiler.updateTime = updateClock.getElapsedTime().asSeconds();
}

void PlayState::draw(sf::RenderWindow& window) {
    sf::Clock renderClock;

    window.setView(cameraManager->getView());
    background->draw(window);
    
    if (lightingManager) lightingManager->drawFog(window);

    if (worldManager) {
        worldManager->drawBackground(window, cameraManager->getViewBounds(), debugOverlay->getShowFoliage(), profiler, game->getAssetManager().getTexture("tileset"));
        worldManager->drawGeometry(window, cameraManager->getViewBounds(), profiler);
        
        if (particleSystem) particleSystem->draw(window);

        if (debugOverlay) {
            worldManager->drawDebug(window, 
                cameraManager->getViewBounds(), 
                cameraManager->getPreloadBounds(player->getVelocity()), 
                cameraManager->getUnloadBounds(),
                debugOverlay.get());
        }
    }
    
    if (player) player->draw(window);
    
    window.setView(window.getDefaultView());
    
    if (lightingManager) lightingManager->drawAmbient(window);
    if (debugOverlay) debugOverlay->draw(window);
    
    profiler.renderTime = renderClock.getElapsedTime().asSeconds();
}