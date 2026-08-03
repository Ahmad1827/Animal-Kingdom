#include "states/PlayState.h"
#include "core/Game.h"
#include "world/Biome.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>

PlayState::PlayState(Game* game) : game(game), f3PressedLastFrame(false), f4PressedLastFrame(false), f5PressedLastFrame(false), f6PressedLastFrame(false), f7PressedLastFrame(false), f8PressedLastFrame(false) {}

void PlayState::init() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    activeSeed = std::rand();
    background = std::make_unique<Background>(game->getAssetManager());
    player = std::make_unique<Ape>(0.f, 0.f, game->getAssetManager().getTexture("playerTex"));
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
    if (f10Pressed && !f10PressedLastFrame) debugOverlay->toggleGenerationDebug(); 
    f10PressedLastFrame = f10Pressed;

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

        // --- DYNAMIC VINE STATE TRACKERS ---
        static uint64_t grabbedChunk = 0;
        static int grabbedVine = -1;
        static int grabbedSeg = -1;
        static float climbTimer = 0.f;

        if (player->getState() != ApeState::ClimbingVine) {
            grabbedVine = -1;
        }

        float preCollisionVelY = player->getVelocity().y;

        sf::Clock physicsClock;
        sf::FloatRect playerBounds = player->getBounds();
        sf::FloatRect platformBounds;
        
        bool wasGrounded = (player->getState() == ApeState::Grounded);

        if (player->getState() != ApeState::ClimbingTrunk && player->getState() != ApeState::HangingBranch && player->getState() != ApeState::ClimbingVine) {
            player->setState(ApeState::Airborne);
        }

        float playerCenterX = playerBounds.left + (playerBounds.width / 2.f);
        float groundHeight = worldManager->getTerrainHeight(playerCenterX);
        float bottomY = playerBounds.top + playerBounds.height;
        float distanceToGround = groundHeight - bottomY;

        if (player->getVelocity().y >= 0.f && bottomY >= groundHeight) {
            player->setPosition(player->getPosition().x, groundHeight - playerBounds.height);
            player->setVelocity(player->getVelocity().x, 0.f);
            player->setState(ApeState::Grounded);
            player->setDroppingThrough(false);
        } 
        else if (wasGrounded && player->getVelocity().y >= 0.f && distanceToGround > 0.f && distanceToGround < 25.f) {
            player->setPosition(player->getPosition().x, groundHeight - playerBounds.height);
            player->setVelocity(player->getVelocity().x, 0.f);
            player->setState(ApeState::Grounded);
            player->setDroppingThrough(false);
        }

        sf::FloatRect checkBounds = playerBounds;
        sf::Vector2f checkVel = player->getVelocity();
        
        if (wasGrounded) {
            checkBounds.top += 2.f; 
            if (checkVel.y == 0.f) checkVel.y = 10.f; 
        }

        if (player->getState() == ApeState::Airborne && !player->isDroppingThrough()) {
            if (worldManager->checkOneWayCollision(checkBounds, checkVel, dt, platformBounds)) {
                player->setPosition(player->getPosition().x, platformBounds.top - playerBounds.height);
                player->setVelocity(player->getVelocity().x, 0.f);
                player->setState(ApeState::Grounded);
            }
        }

        if (!wasGrounded && player->getState() == ApeState::Grounded) {
            ImpactLevel impact = player->registerLanding(preCollisionVelY);
            sf::Vector2f spawnPos = player->getPosition() + sf::Vector2f(playerBounds.width/2.f, playerBounds.height);
            
            if (impact == ImpactLevel::Heavy) {
                cameraManager->addTrauma(0.8f);
                particleSystem->spawnImpactLeaves(spawnPos, 20); 
            } else if (impact == ImpactLevel::Medium) {
                cameraManager->addTrauma(0.4f);
                particleSystem->spawnImpactLeaves(spawnPos, 10);
            } else if (impact == ImpactLevel::Light) {
                cameraManager->addTrauma(0.15f);
                particleSystem->spawnImpactLeaves(spawnPos, 4);
            }
        }
        
        if (std::abs(player->getVelocity().x) > 10.f) {
            worldManager->disturbEnvironment(playerBounds, player->getVelocity().x);
        }

        float trunkCenter = 0.f;
        uint64_t tChunk = 0;
        int tVine = -1, tSeg = -1;
        
        if (worldManager->checkTrunkCollision(playerBounds, trunkCenter)) {
            if ((sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) && 
                player->getState() != ApeState::ClimbingTrunk && 
                player->getState() != ApeState::HangingBranch) {
                player->setState(ApeState::ClimbingTrunk);
                player->setPosition(trunkCenter - (playerBounds.width / 2.f), player->getPosition().y);
                player->setVelocity(0.f, 0.f);
            }
        } 
        // --- NEW DYNAMIC VINE HANDLING ---
        else if (grabbedVine != -1) { 
            sf::Vector2f segPos = worldManager->getVineSegmentPosition(grabbedChunk, grabbedVine, grabbedSeg);
            player->setPosition(segPos.x - playerBounds.width / 2.f, segPos.y - playerBounds.height / 2.f);
            
            sf::Vector2f vineVel = worldManager->getVineSegmentVelocity(grabbedChunk, grabbedVine, grabbedSeg, dt);
            
            // FIX: Capped maximum swing speed + responsive pushing
            float maxSwingSpeed = 250.f; 
            float swingPush = 700.f * dt; 
            
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                if (vineVel.x > -maxSwingSpeed) {
                    worldManager->applyVineForce(grabbedChunk, grabbedVine, grabbedSeg, sf::Vector2f(-swingPush, 0.f));
                }
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                if (vineVel.x < maxSwingSpeed) {
                    worldManager->applyVineForce(grabbedChunk, grabbedVine, grabbedSeg, sf::Vector2f(swingPush, 0.f));
                }
            }

            climbTimer += dt;
            if (climbTimer > 0.1f) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                    // FIX: Never climb onto Segment 0, because it is the unmoving anchor!
                    if (grabbedSeg > 1) grabbedSeg--; 
                    climbTimer = 0.f;
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                    int maxSegs = worldManager->getVineSegmentCount(grabbedChunk, grabbedVine);
                    if (grabbedSeg < maxSegs - 1) grabbedSeg++;
                    climbTimer = 0.f;
                }
            }

            player->setVelocity(vineVel.x, 0.f);

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                player->setState(ApeState::Airborne);
                // FIX: Cap the slingshot velocity so you don't fly out of bounds
                float jumpOffX = std::clamp(vineVel.x * 1.5f, -350.f, 350.f);
                player->setVelocity(jumpOffX, -500.f); 
                grabbedVine = -1;
            }
        } 
        else if (worldManager->checkVineCollision(playerBounds, tChunk, tVine, tSeg)) {
            if ((sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) && 
                player->getState() != ApeState::ClimbingVine) {
                
                player->setState(ApeState::ClimbingVine);
                grabbedChunk = tChunk;
                grabbedVine = tVine;
                // FIX: Don't allow grabbing the static anchor block
                grabbedSeg = (tSeg == 0) ? 1 : tSeg; 
                climbTimer = 0.f;
                
                // FIX: Instantly transfer jumping momentum into the vine so it catches your weight
                float transferForce = std::clamp(player->getVelocity().x * 0.015f, -8.f, 8.f);
                worldManager->applyVineForce(grabbedChunk, grabbedVine, grabbedSeg, sf::Vector2f(transferForce, 0.f));
            }
        } 
        else {
            if (player->getState() == ApeState::ClimbingTrunk || player->getState() == ApeState::ClimbingVine) {
                player->setState(ApeState::Airborne);
            }
        }

        sf::FloatRect branchBounds;
        if (player->getState() != ApeState::ClimbingTrunk && player->getState() != ApeState::ClimbingVine && player->getState() != ApeState::Grounded) {
            
            sf::FloatRect hangCheckBounds = playerBounds;
            if (player->getState() == ApeState::HangingBranch) {
                hangCheckBounds.top -= 2.f; 
            }

            if (worldManager->checkHangCollision(hangCheckBounds, branchBounds)) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                    player->setState(ApeState::HangingBranch);
                    player->setPosition(player->getPosition().x, branchBounds.top + branchBounds.height - 2.f);
                    if (player->getVelocity().y < 0.f) player->setVelocity(player->getVelocity().x, 0.f);
                }

                if (player->getState() == ApeState::HangingBranch) {
                    float visualPadding = 25.f; 
                    float leftLimit = branchBounds.left + visualPadding;
                    float rightLimit = branchBounds.left + branchBounds.width - playerBounds.width - visualPadding;

                    if (leftLimit > rightLimit) {
                         float center = branchBounds.left + branchBounds.width/2.f - playerBounds.width/2.f;
                         leftLimit = rightLimit = center;
                    }

                    float currentX = player->getPosition().x;
                    if (currentX < leftLimit) {
                        player->setPosition(leftLimit, player->getPosition().y);
                        player->setVelocity(0.f, 0.f);
                    } else if (currentX > rightLimit) {
                        player->setPosition(rightLimit, player->getPosition().y);
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
        profiler.animName = player->getAnimator()->getCurrentAnimationName();
        profiler.animFrame = player->getAnimator()->getCurrentFrame();
        profiler.animTime = player->getAnimator()->getCurrentTime();
        profiler.animFPS = player->getAnimator()->getFPS();
        profiler.currentDt = dt;
        profiler.playerStateInt = static_cast<int>(player->getState());
        
        if (player->getAnimator()) {
            profiler.animName = player->getAnimator()->getCurrentAnimationName();
            profiler.animFrame = player->getAnimator()->getCurrentFrame();
            profiler.animTime = player->getAnimator()->getCurrentTime();
            profiler.animFPS = player->getAnimator()->getFPS();
            profiler.animRect = player->getAnimator()->getCurrentRect();
            profiler.animOffset = player->getAnimator()->getCurrentOffset();
            profiler.spriteScale = player->getSprite().getScale();
            profiler.spritePos = player->getSprite().getPosition();
            profiler.spriteOrigin = player->getSprite().getOrigin();
        }
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