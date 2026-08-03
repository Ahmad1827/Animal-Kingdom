#include "states/PlayState.h"
#include "core/Game.h"
#include "world/Biome.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>
#include <algorithm>

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

    sceneBuffer.create(1280, 720);
    if (sf::Shader::isAvailable()) {
        if (crtShader.loadFromFile("assets/shaders/crt.frag", sf::Shader::Fragment)) {
            crtShader.setUniform("texture", sf::Shader::CurrentTexture);
            crtShader.setUniform("resolution", sf::Vector2f(1280.f, 720.f));
        }
    }
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

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) cameraManager->setZoom(0.5f);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) cameraManager->setZoom(2.0f);
    else cameraManager->setZoom(1.35f);

    if (player) {
        player->update(dt);

        static uint64_t grabbedChunk = 0;
        static int grabbedVine = -1;
        static int grabbedSeg = -1;
        static sf::FloatRect activeBranch;
        static bool isDroppingToHang = false;

        if (player->getState() != ApeState::ClimbingVine) {
            grabbedVine = -1;
        }

        float preCollisionVelY = player->getVelocity().y;
        sf::Clock physicsClock;
        sf::FloatRect playerBounds = player->getBounds();
        sf::FloatRect platformBounds;
        
        bool wasGrounded = (player->getState() == ApeState::Grounded);

        if (wasGrounded && (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))) {
            sf::FloatRect dropCheck = playerBounds;
            dropCheck.top += playerBounds.height;
            dropCheck.height = 20.f; 
            sf::FloatRect branchBounds;
            
            if (worldManager->checkHangCollision(dropCheck, branchBounds)) {
                isDroppingToHang = true;
                activeBranch = branchBounds;
                player->setState(ApeState::Airborne);
                player->setDroppingThrough(true);
                player->setVelocity(player->getVelocity().x, 150.f);
                wasGrounded = false; 
            } else {
                player->setDroppingThrough(true);
            }
        }

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
            isDroppingToHang = false;
        } 
        else if (wasGrounded && player->getVelocity().y >= 0.f && distanceToGround > 0.f && distanceToGround < 25.f) {
            player->setPosition(player->getPosition().x, groundHeight - playerBounds.height);
            player->setVelocity(player->getVelocity().x, 0.f);
            player->setState(ApeState::Grounded);
            player->setDroppingThrough(false);
            isDroppingToHang = false;
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
                isDroppingToHang = false;
            }
        }

        if (isDroppingToHang) {
            float branchBottom = activeBranch.top + activeBranch.height;
            float targetY = branchBottom + 120.f; 
            float apeCenterXDrop = player->getPosition().x + (player->getBounds().width / 2.f);

            bool isUnderBranch = (apeCenterXDrop >= activeBranch.left && apeCenterXDrop <= activeBranch.left + activeBranch.width);

            if (player->getPosition().y >= targetY && isUnderBranch) { 
                player->setState(ApeState::HangingBranch);
                player->setPosition(player->getPosition().x, targetY);
                player->setVelocity(player->getVelocity().x, 0.f);
                player->setDroppingThrough(false);
                isDroppingToHang = false;
            } else if (!isUnderBranch || player->getState() == ApeState::Grounded) {
                isDroppingToHang = false;
                player->setDroppingThrough(false);
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
        
        if (std::abs(player->getVelocity().x) > 10.f && player->getState() != ApeState::ClimbingVine) {
            worldManager->disturbEnvironment(playerBounds, player->getVelocity().x);
        }

        float trunkCenter = 0.f;
        float trunkTop = 0.f;
        uint64_t tChunk = 0;
        int tVine = -1, tSeg = -1;
        
        bool touchingTrunk = worldManager->checkTrunkCollision(playerBounds, trunkCenter, trunkTop);
        
        if (touchingTrunk) {
            if ((sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) && 
                player->getState() != ApeState::ClimbingTrunk && 
                player->getState() != ApeState::HangingBranch) {
                
                if (player->getPosition().y >= trunkTop - 10.f) {
                    player->setState(ApeState::ClimbingTrunk);
                    player->setPosition(trunkCenter - (playerBounds.width / 2.f), player->getPosition().y);
                    player->setVelocity(0.f, 0.f);
                    isDroppingToHang = false;
                }
            }
            
            if (player->getState() == ApeState::ClimbingTrunk) {
                if (player->getPosition().y < trunkTop) {
                    player->setPosition(player->getPosition().x, trunkTop);
                    if (player->getVelocity().y < 0.f) {
                        player->setVelocity(0.f, 0.f);
                    }
                }
            }
        } else if (player->getState() == ApeState::ClimbingTrunk) {
            player->setState(ApeState::Airborne);
        }
        
        if (grabbedVine != -1) { 
            if (worldManager->getVineSegmentCount(grabbedChunk, grabbedVine) == 0) {
                grabbedVine = -1;
                player->setState(ApeState::Airborne);
            } else {
                sf::Vector2f vineVel = worldManager->getVineSegmentVelocity(grabbedChunk, grabbedVine, grabbedSeg, dt);
                
                float maxSwingSpeed = 200.f; 
                float swingPush = 400.f * dt; 
                
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                    if (vineVel.x > -maxSwingSpeed) {
                        worldManager->applyVineForce(grabbedChunk, grabbedVine, grabbedSeg, sf::Vector2f(-swingPush, 0.f));
                    }
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                    if (vineVel.x < maxSwingSpeed) {
                        worldManager->applyVineForce(grabbedChunk, grabbedVine, grabbedSeg, sf::Vector2f(swingPush, 0.f));
                    }
                }

                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                    player->setState(ApeState::Airborne);
                    float jumpOffX = std::clamp(vineVel.x * 1.5f, -400.f, 400.f);
                    player->setVelocity(jumpOffX, -500.f); 
                    grabbedVine = -1;
                }
            }
        } 
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::G) && player->getState() != ApeState::ClimbingVine) {
            if (worldManager->checkVineCollision(playerBounds, tChunk, tVine, tSeg)) {
                player->setState(ApeState::ClimbingVine);
                grabbedChunk = tChunk;
                grabbedVine = tVine;
                isDroppingToHang = false;
                
                float pCenterY = playerBounds.top + playerBounds.height / 2.f;
                int bestSeg = 1;
                float bestDist = 99999.f;
                int segCount = worldManager->getVineSegmentCount(tChunk, tVine);
                for (int i = 1; i < segCount; ++i) {
                    float sY = worldManager->getVineSegmentPosition(tChunk, tVine, i).y;
                    float expectedY = sY + 120.f;
                    float dist = std::abs(expectedY - playerBounds.top);
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestSeg = i;
                    }
                }
                
                while (bestSeg > 1) {
                    float sY = worldManager->getVineSegmentPosition(tChunk, tVine, bestSeg).y;
                    if (sY + 120.f + playerBounds.height >= groundHeight - 5.f) {
                        bestSeg--;
                    } else {
                        break;
                    }
                }
                
                grabbedSeg = bestSeg;
                float transferForce = std::clamp(player->getVelocity().x * 0.02f, -12.f, 12.f);
                worldManager->applyVineForce(grabbedChunk, grabbedVine, grabbedSeg, sf::Vector2f(transferForce, 0.f));
            }
        }

        if (player->getState() == ApeState::HangingBranch) {
            float apeW = playerBounds.width;
            float apeCenter = player->getPosition().x + (apeW / 2.f);
            
            float leftLimit = activeBranch.left + 5.f; 
            float rightLimit = activeBranch.left + activeBranch.width - 5.f;
            
            if (leftLimit > rightLimit) {
                 leftLimit = rightLimit = activeBranch.left + (activeBranch.width / 2.f);
            }

            if (apeCenter < leftLimit) {
                player->setPosition(leftLimit - (apeW / 2.f), player->getPosition().y);
                player->setVelocity(0.f, 0.f);
            } else if (apeCenter > rightLimit) {
                player->setPosition(rightLimit - (apeW / 2.f), player->getPosition().y);
                player->setVelocity(0.f, 0.f);
            }
            
            player->setPosition(player->getPosition().x, activeBranch.top + activeBranch.height + 120.f);
        }
        
        profiler.physicsTime = physicsClock.getElapsedTime().asSeconds();

        sf::Clock cameraClock; 
        cameraManager->update(dt, player->getPosition(), player->getVelocity(), player->getState());
        profiler.cameraTime = cameraClock.getElapsedTime().asSeconds();

        sf::FloatRect preloadBounds = cameraManager->getPreloadBounds(player->getVelocity());
        sf::FloatRect unloadBounds = cameraManager->getUnloadBounds();

        if (worldManager) {
            worldManager->updateSway(dt, cameraManager->getViewBounds(), weatherManager->getWindVector());
            worldManager->update(dt, preloadBounds, unloadBounds, profiler);
        }

        if (grabbedVine != -1 && player->getState() == ApeState::ClimbingVine) {
            sf::Vector2f segPos = worldManager->getVineSegmentPosition(grabbedChunk, grabbedVine, grabbedSeg);
            
            float targetY = segPos.y + 120.f;
            if (targetY + playerBounds.height > groundHeight) {
                targetY = groundHeight - playerBounds.height;
            }
            
            player->setPosition(segPos.x - (playerBounds.width / 2.f), targetY);
            
            sf::Vector2f vineVel = worldManager->getVineSegmentVelocity(grabbedChunk, grabbedVine, grabbedSeg, dt);
            player->setVelocity(vineVel.x, 0.f);
        }

        sf::Clock pClock;
        weatherManager->update(dt);
        particleSystem->update(dt, cameraManager->getViewBounds(), weatherManager->getWindVector(), weatherManager->getRainIntensity(), worldClock->getTimeOfDay());
        profiler.particleTime = pClock.getElapsedTime().asSeconds();
        
        if (audioManager) audioManager->update(dt, weatherManager->getWindIntensity(), weatherManager->getRainIntensity(), worldClock->getTimeOfDay());
        lightingManager->update(dt, cameraManager->getView(), worldClock->getTimeOfDay(), weatherManager->getFogDensity());

        background->update(
            cameraManager->getViewBounds().left + cameraManager->getViewBounds().width / 2.f,
            cameraManager->getViewBounds().top + cameraManager->getViewBounds().height / 2.f,
            cameraManager->getView().getSize(),
            dt
        );

        profiler.playerPos = player->getPosition();
        profiler.cameraPos = cameraManager->getView().getCenter();
        profiler.cameraTarget = cameraManager->getIdealPosition();
        profiler.groundHeight = groundHeight;
        profiler.verticalVelocity = player->getVelocity().y;
        profiler.isGrounded = (player->getState() == ApeState::Grounded);
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

    sceneBuffer.setView(cameraManager->getView());
    sceneBuffer.clear();

    background->draw(sceneBuffer);
    
    if (lightingManager) lightingManager->drawFog(sceneBuffer);

    if (worldManager) {
        worldManager->drawBackground(sceneBuffer, cameraManager->getViewBounds(), debugOverlay->getShowFoliage(), profiler, game->getAssetManager().getTexture("tileset"));
        worldManager->drawGeometry(sceneBuffer, cameraManager->getViewBounds(), profiler);
        
        if (particleSystem) particleSystem->draw(sceneBuffer);

        if (debugOverlay) {
            worldManager->drawDebug(sceneBuffer, 
                cameraManager->getViewBounds(), 
                cameraManager->getPreloadBounds(player->getVelocity()), 
                cameraManager->getUnloadBounds(),
                debugOverlay.get());
        }
    }
    
    if (player) player->draw(sceneBuffer);
    
    sceneBuffer.setView(sceneBuffer.getDefaultView());
    if (lightingManager) lightingManager->drawAmbient(sceneBuffer);

    sceneBuffer.display();

    window.setView(window.getDefaultView());
    sf::Sprite sceneSprite(sceneBuffer.getTexture());

    if (sf::Shader::isAvailable()) {
        window.draw(sceneSprite, &crtShader);
    } else {
        window.draw(sceneSprite);
    }

    if (debugOverlay) debugOverlay->draw(window);
    
    profiler.renderTime = renderClock.getElapsedTime().asSeconds();
}