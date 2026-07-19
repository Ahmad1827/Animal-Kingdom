#include "states/PlayState.h"
#include "core/Game.h"
#include "world/Biome.h"
#include <cstdlib>
#include <ctime>

PlayState::PlayState(Game* game) : game(game), f3PressedLastFrame(false), f4PressedLastFrame(false), f5PressedLastFrame(false), f6PressedLastFrame(false), f7PressedLastFrame(false), f8PressedLastFrame(false) {}

void PlayState::init() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    activeSeed = std::rand();

    player = std::make_unique<Ape>(0.f, 0.f);
    worldManager = std::make_unique<WorldManager>(activeSeed);
    cameraManager = std::make_unique<CameraManager>(sf::Vector2f(1280.f, 720.f));
    background = std::make_unique<ParallaxBackground>();
    lightingManager = std::make_unique<LightingManager>();
    weatherManager = std::make_unique<WeatherManager>();
    particleSystem = std::make_unique<ParticleSystem>();
    audioManager = std::make_unique<AudioManager>();
    worldClock = std::make_unique<WorldClock>();
    debugOverlay = std::make_unique<DebugOverlay>();
    
    // Set default day length fast for testing cycles
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

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) cameraManager->setZoom(0.5f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) cameraManager->setZoom(2.0f);
    else cameraManager->setZoom(1.35f);

    if (player) {
        // 1. Input & Physics
        player->update(dt);

        sf::Clock collisionClock;
        sf::FloatRect playerBounds = player->getBounds();
        sf::FloatRect platformBounds;
        
        // 2. Collision Resolution BEFORE Camera Update (Fixes camera jitter)
        if (player->getState() != ApeState::ClimbingTrunk && player->getState() != ApeState::HangingBranch && player->getState() != ApeState::ClimbingVine) {
            player->setState(ApeState::Airborne);
        }

        float playerCenterX = playerBounds.left + (playerBounds.width / 2.f);
        float groundHeight = worldManager->getTerrainHeight(playerCenterX);

        ApeState previousState = player->getState();

        if (player->getVelocity().y > 0.f && (playerBounds.top + playerBounds.height) >= groundHeight) {
            player->setPosition(player->getPosition().x, groundHeight - playerBounds.height);
            player->setVelocity(player->getVelocity().x, 0.f);
            player->setState(ApeState::Grounded);
            player->setDroppingThrough(false);
        }

        if (player->getState() == ApeState::Airborne && !player->isDroppingThrough()) {
            if (worldManager->checkOneWayCollision(playerBounds, player->getVelocity(), dt, platformBounds)) {
                player->setPosition(player->getPosition().x, platformBounds.top - playerBounds.height);
                player->setVelocity(player->getVelocity().x, 0.f);
                player->setState(ApeState::Grounded);
            }
        }

        // Environmental Physical Impact (Leaves fall on hard landing)
        if (previousState == ApeState::Airborne && player->getState() == ApeState::Grounded && std::abs(player->getVelocity().y) > 300.f) {
            particleSystem->spawnImpactLeaves(player->getPosition() + sf::Vector2f(playerBounds.width/2.f, playerBounds.height), 8);
        }
        
        // Vines swing when player moves through them
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
        
        profiler.collisionTime = collisionClock.getElapsedTime().asSeconds();

        // 3. Camera Update AFTER precise player positioning
        cameraManager->update(dt, player->getPosition(), player->getVelocity(), player->getState());

        // 4. Streaming based on exactly resolved camera bounds
        sf::FloatRect preloadBounds = cameraManager->getPreloadBounds(player->getVelocity());
        sf::FloatRect unloadBounds = cameraManager->getUnloadBounds();

        if (worldManager) {
            worldManager->update(dt, preloadBounds, unloadBounds, profiler);
        }

        // 5. Sway and Particle Updates
        // 5. Sway and Particle Updates
        // 5. Sway and Particle Updates
        sf::Clock pClock;
        weatherManager->update(dt);
        worldManager->updateSway(dt, cameraManager->getViewBounds(), weatherManager->getWindVector());
        particleSystem->update(dt, cameraManager->getViewBounds(), weatherManager->getWindVector(), weatherManager->getRainIntensity(), worldClock->getTimeOfDay());
        profiler.particleTime = pClock.getElapsedTime().asSeconds();
        
        audioManager->update(dt, weatherManager->getWindIntensity(), weatherManager->getRainIntensity(), worldClock->getTimeOfDay());
        background->update(cameraManager->getView().getCenter().x, cameraManager->getView().getCenter().y);
        lightingManager->update(dt, cameraManager->getView(), worldClock->getTimeOfDay(), weatherManager->getFogDensity());

        // Debug info rendering
        profiler.weatherString = weatherManager->getWeatherString();
        profiler.seasonString = weatherManager->getSeasonString();
        profiler.particleCount = particleSystem->getParticleCount();
        profiler.timeOfDay = worldClock->getTimeOfDay();

        // Debug info rendering
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

    if (background) background->draw(window);

    window.setView(cameraManager->getView());
    
    if (lightingManager) lightingManager->drawFog(window);

    if (worldManager) {
        worldManager->drawBackground(window, cameraManager->getViewBounds(), debugOverlay ? debugOverlay->getShowFoliage() : true, profiler);
        worldManager->drawGeometry(window, cameraManager->getViewBounds(), profiler);
        
        if (particleSystem) particleSystem->draw(window);

        if (debugOverlay) {
            worldManager->drawDebug(window, 
                cameraManager->getViewBounds(), 
                cameraManager->getPreloadBounds(player->getVelocity()), 
                cameraManager->getUnloadBounds(),
                debugOverlay->getShowBorders(), 
                debugOverlay->getShowRegions(), 
                debugOverlay->getShowHeatmaps());
        }
    }
    
    if (player) player->draw(window);
    
    window.setView(window.getDefaultView());
    
    if (lightingManager) lightingManager->drawAmbient(window);
    if (debugOverlay) debugOverlay->draw(window);
    
    profiler.renderTime = renderClock.getElapsedTime().asSeconds();
}