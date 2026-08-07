#include "states/PlayState.h"
#include "core/Game.h"
#include "world/Biome.h"
#include "simulation/PopulationGenerator.h"
#include "simulation/SuccessionManager.h"
#include "simulation/KingdomManager.h"
#include "simulation/WarfareManager.h"
#include "world/targets/BonfireInteractionTarget.hpp"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>
#include <algorithm>
#include "world/targets/VillageCenterInteractionTarget.hpp"
#include "world/targets/StorageHutInteractionTarget.hpp"
#include "world/targets/KingInteractionTarget.hpp"
#include "world/targets/BorderTotemInteractionTarget.hpp"

PlayState::PlayState(Game* game) : game(game), isTransitioning(false), transitionTimer(0.f), f3PressedLastFrame(false), f4PressedLastFrame(false), f5PressedLastFrame(false), f6PressedLastFrame(false), f7PressedLastFrame(false), f8PressedLastFrame(false), f9PressedLastFrame(false), f10PressedLastFrame(false), f11PressedLastFrame(false) {}

void PlayState::init() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    activeSeed = std::rand();
    background = std::make_unique<Background>(game->getAssetManager());
    worldManager = std::make_unique<WorldManager>(activeSeed, game->getAssetManager().getTexture("decors"));
    cameraManager = std::make_unique<CameraManager>(sf::Vector2f(1280.f, 720.f));
    lightingManager = std::make_unique<LightingManager>();
    weatherManager = std::make_unique<WeatherManager>();
    particleSystem = std::make_unique<ParticleSystem>();
    audioManager = std::make_unique<AudioManager>();
    worldClock = std::make_unique<WorldClock>();
    debugOverlay = std::make_unique<DebugOverlay>();
    
    simulationManager = std::make_unique<sim::SimulationManager>();
    structureManager = std::make_unique<StructureManager>();
    
    cinematicFont.loadFromFile("font.ttf");

    sim::EntityID startApeId = sim::PopulationGenerator::generatePlayerDynasty(simulationManager->getRegistry(), activeSeed);
    simulationManager->setControlledApe(startApeId);
    sim::PopulationGenerator::generateVillages(simulationManager->getRegistry(), activeSeed);
    
    npcManager = std::make_unique<NPCManager>(game->getAssetManager().getTexture("playerTex"));
    
    sim::ApeData* pData = simulationManager->getRegistry().getApe(startApeId);
    if (pData) {
        playerWrapper = std::make_unique<Ape>(pData->worldX, pData->worldY, game->getAssetManager().getTexture("playerTex"), true);
    }
    

    
    worldClock->setMultiplier(30.f);
}

void PlayState::processEvents(const sf::Event& event) {
    // Route events through the interaction manager first
    interactionManager.handleEvent(event, *cameraManager);

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::K) {
            debugOverlay->toggleKingdomDebug();
        }
        if (event.key.code == sf::Keyboard::F6) {
            sim::ApeData* cData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
            if (cData && cData->villageId != 0 && cData->currentKingdom == 0) {
                sim::KingdomManager::spawnDebugKingdom(simulationManager->getRegistry(), cData->villageId, cData->dynastyId, cData->id);
            }
        }
        if (event.key.code == sf::Keyboard::F7) {
            sim::ApeData* cData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
            if (cData) sim::SuccessionManager::handleDeath(simulationManager->getRegistry(), cData->id);
        }
        if (event.key.code == sf::Keyboard::N) {
            debugOverlay->toggleWarfareDebug();
        }
        if (event.key.code == sf::Keyboard::F12) {
            sim::ApeData* cData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
            if (cData && cData->currentKingdom != 0) {
                sim::KingdomData* myK = simulationManager->getRegistry().getKingdom(cData->currentKingdom);
                if (myK && !myK->knownKingdoms.empty()) {
                    sim::KingdomID targetK = *myK->knownKingdoms.begin();
                    sim::WarfareManager::declareWar(simulationManager->getRegistry(), myK->id, targetK, "Royal decree by player.");
                }
            }
        }
        if (event.key.code == sf::Keyboard::M) {
            sim::ApeData* cData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
            if (cData && cData->currentKingdom != 0) {
                sim::KingdomData* myK = simulationManager->getRegistry().getKingdom(cData->currentKingdom);
                if (myK && myK->currentKingId == cData->id && !myK->knownKingdoms.empty()) {
                    sim::KingdomID targetK = *myK->knownKingdoms.begin();
                    sim::WarfareManager::issueMusterOrder(simulationManager->getRegistry(), myK->id, targetK, cData->id);
                }
            }
        }
    }
}

void PlayState::update(float dt) {
    size_t currentKingdomCount = simulationManager->getRegistry().getAllKingdoms().size();
    if (currentKingdomCount != lastKingdomCount) {
        refreshInteractionTargets();
        lastKingdomCount = currentKingdomCount;
    }
    static uint64_t grabbedChunk = 0;
    static int grabbedVine = -1;
    static int grabbedSeg = -1;
    static sf::FloatRect activeBranch;
    static bool isDroppingToHang = false;
    static float climbTimer = 0.f;

    profiler.resetPerFrame();
    profiler.fps = (dt > 0.f) ? 1.f / dt : 0.f;
    profiler.frameTime = dt * 1000.f;
    
    sf::Clock updateClock;
    worldClock->update(dt);

    if (simulationManager) {
        simulationManager->update(dt * 30.f);
    }

    bool f3Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::O);
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

    bool f11Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::V);
    if (f11Pressed && !f11PressedLastFrame) debugOverlay->toggleVillageDebug();
    f11PressedLastFrame = f11Pressed;

    bool f9Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F9);
    if (f9Pressed && !f9PressedLastFrame && !isTransitioning) {
        sim::ApeData* cData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
        if (cData) {
            sim::DynastyData* dyn = simulationManager->getRegistry().getDynasty(cData->dynastyId);
            if (dyn) {
                bool foundCurrent = false;
                sim::EntityID nextId = 0;
                for (auto id : dyn->members) {
                    sim::ApeData* mem = simulationManager->getRegistry().getApe(id);
                    if (!mem || !mem->alive) continue;
                    if (foundCurrent) {
                        nextId = id;
                        break;
                    }
                    if (id == cData->id) foundCurrent = true;
                }
                if (nextId == 0) {
                    for (auto id : dyn->members) {
                        sim::ApeData* mem = simulationManager->getRegistry().getApe(id);
                        if (mem && mem->alive && id != cData->id) {
                            nextId = id;
                            break;
                        }
                    }
                }
                if (nextId != 0) {
                    simulationManager->setControlledApe(nextId);
                    isTransitioning = true;
                    transitionTimer = 2.0f;
                    playerWrapper.reset();
                }
            }
        }
    }
    f9PressedLastFrame = f9Pressed;

    bool f10Pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::F10);
    if (f10Pressed && !f10PressedLastFrame) {
        sim::ApeData* cData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
        if (cData) {
            sim::DynastyData* d = simulationManager->getRegistry().getDynasty(cData->dynastyId);
            if (d) {
                std::cout << "--- DYNASTY " << d->name << " ---\n";
                for (auto m : d->members) {
                    sim::ApeData* a = simulationManager->getRegistry().getApe(m);
                    if (a) std::cout << "Member ID " << m << ": " << a->name << (a->alive ? " (Alive)" : " (Dead)") << "\n";
                }
            }
        }
    }
    f10PressedLastFrame = f10Pressed;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::K) && playerWrapper && !isTransitioning) {
        sim::ApeData* current = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
        if (current) current->alive = false;
    }

    // Only allow camera zooming if not interacting
    if (!interactionManager.isInteracting()) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) cameraManager->setZoom(0.5f);
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) cameraManager->setZoom(2.0f);
        else cameraManager->setZoom(1.35f);
    }

    if (!isTransitioning && playerWrapper) {
        sim::ApeData* pData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
        if (pData && !pData->alive) {
            sim::EntityID heirId = sim::SuccessionManager::findNextHeir(simulationManager->getRegistry(), pData->id);
            if (heirId != 0) {
                simulationManager->setControlledApe(heirId);
                isTransitioning = true;
                transitionTimer = 3.0f;
                playerWrapper.reset();
            } else {
                std::cout << "DYNASTY EXTINCT. GAME OVER.\n";
            }
        }
    }

    if (isTransitioning) {
        transitionTimer -= dt;
        sim::ApeData* targetData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
        if (targetData) {
            transitionTarget = sf::Vector2f(targetData->worldX, targetData->worldY);
            cameraManager->updateTransition(dt, transitionTarget);
        }
        
        if (transitionTimer <= 0.f) {
            isTransitioning = false;
            if (targetData) {
                npcManager->removeNPC(targetData->id);
                playerWrapper = std::make_unique<Ape>(targetData->worldX, targetData->worldY, game->getAssetManager().getTexture("playerTex"), true);
            }
        }
    }

    if (!isTransitioning && playerWrapper) {
        // Universal interaction framework update
        interactionManager.update(dt, playerWrapper->getPosition(), *cameraManager);

        // Update player physical state
        playerWrapper->update(dt);
        
        // Zero out horizontal velocity if stuck in an interaction menu to prevent wandering off
        if (interactionManager.isInteracting()) {
            playerWrapper->setVelocity(0.f, playerWrapper->getVelocity().y);
        }
        
        sim::ApeData* pData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
        if (pData) {
            pData->worldX = playerWrapper->getPosition().x;
            pData->worldY = playerWrapper->getPosition().y;
            pData->currentChunkX = static_cast<int>(std::floor(pData->worldX / 2000.f));
            
            bool isKing = false;
            if (pData->currentKingdom != 0) {
                sim::KingdomData* kd = simulationManager->getRegistry().getKingdom(pData->currentKingdom);
                if (kd && kd->currentKingId == pData->id) isKing = true;
            }
            
            playerWrapper->setVisualEquipment(pData->equippedTool, pData->carriedType, pData->carriedAmount, isKing);
        }

        if (playerWrapper->getState() != ApeState::ClimbingVine) {
            grabbedVine = -1;
        }

        float preCollisionVelY = playerWrapper->getVelocity().y;
        sf::Clock physicsClock;
        sf::FloatRect playerBounds = playerWrapper->getBounds();
        sf::FloatRect platformBounds;
        
        bool wasGrounded = (playerWrapper->getState() == ApeState::Grounded);

        // Disallow player drop-down through branches if interacting
        if (wasGrounded && !interactionManager.isInteracting() && (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))) {
            sf::FloatRect dropCheck = playerBounds;
            dropCheck.top += playerBounds.height;
            dropCheck.height = 20.f; 
            sf::FloatRect branchBounds;
            
            if (worldManager->checkHangCollision(dropCheck, branchBounds)) {
                isDroppingToHang = true;
                activeBranch = branchBounds;
                playerWrapper->setState(ApeState::Airborne);
                playerWrapper->setDroppingThrough(true);
                playerWrapper->setVelocity(playerWrapper->getVelocity().x, 150.f);
                wasGrounded = false; 
            } else {
                playerWrapper->setDroppingThrough(true);
            }
        }

        if (playerWrapper->getState() != ApeState::ClimbingTrunk && playerWrapper->getState() != ApeState::HangingBranch && playerWrapper->getState() != ApeState::ClimbingVine) {
            playerWrapper->setState(ApeState::Airborne);
        }

        float playerCenterX = playerBounds.left + (playerBounds.width / 2.f);
        float groundHeight = worldManager->getTerrainHeight(playerCenterX);
        float bottomY = playerBounds.top + playerBounds.height;
        float distanceToGround = groundHeight - bottomY;

        if (playerWrapper->getVelocity().y >= 0.f && bottomY >= groundHeight) {
            playerWrapper->setPosition(playerWrapper->getPosition().x, groundHeight - playerBounds.height);
            playerWrapper->setVelocity(playerWrapper->getVelocity().x, 0.f);
            playerWrapper->setState(ApeState::Grounded);
            playerWrapper->setDroppingThrough(false);
            isDroppingToHang = false;
        } 
        else if (wasGrounded && playerWrapper->getVelocity().y >= 0.f && distanceToGround > 0.f && distanceToGround < 25.f) {
            playerWrapper->setPosition(playerWrapper->getPosition().x, groundHeight - playerBounds.height);
            playerWrapper->setVelocity(playerWrapper->getVelocity().x, 0.f);
            playerWrapper->setState(ApeState::Grounded);
            playerWrapper->setDroppingThrough(false);
            isDroppingToHang = false;
        }

        sf::FloatRect checkBounds = playerBounds;
        sf::Vector2f checkVel = playerWrapper->getVelocity();
        
        if (wasGrounded) {
            checkBounds.top += 2.f; 
            if (checkVel.y == 0.f) checkVel.y = 10.f; 
        }

        if (playerWrapper->getState() == ApeState::Airborne && !playerWrapper->isDroppingThrough()) {
            if (worldManager->checkOneWayCollision(checkBounds, checkVel, dt, platformBounds)) {
                playerWrapper->setPosition(playerWrapper->getPosition().x, platformBounds.top - playerBounds.height);
                playerWrapper->setVelocity(playerWrapper->getVelocity().x, 0.f);
                playerWrapper->setState(ApeState::Grounded);
                isDroppingToHang = false;
            }
        }

        if (isDroppingToHang) {
            float branchBottom = activeBranch.top + activeBranch.height;
            float targetY = branchBottom + 120.f; 
            float apeCenterXDrop = playerWrapper->getPosition().x + (playerWrapper->getBounds().width / 2.f);

            bool isUnderBranch = (apeCenterXDrop >= activeBranch.left && apeCenterXDrop <= activeBranch.left + activeBranch.width);

            if (playerWrapper->getPosition().y >= targetY && isUnderBranch) { 
                playerWrapper->setState(ApeState::HangingBranch);
                playerWrapper->setPosition(playerWrapper->getPosition().x, targetY);
                playerWrapper->setVelocity(playerWrapper->getVelocity().x, 0.f);
                playerWrapper->setDroppingThrough(false);
                isDroppingToHang = false;
            } else if (!isUnderBranch || playerWrapper->getState() == ApeState::Grounded) {
                isDroppingToHang = false;
                playerWrapper->setDroppingThrough(false);
            }
        }

        if (!wasGrounded && playerWrapper->getState() == ApeState::Grounded) {
            ImpactLevel impact = playerWrapper->registerLanding(preCollisionVelY);
            sf::Vector2f spawnPos = playerWrapper->getPosition() + sf::Vector2f(playerBounds.width/2.f, playerBounds.height);
            
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
        
        if (std::abs(playerWrapper->getVelocity().x) > 10.f && playerWrapper->getState() != ApeState::ClimbingVine) {
            worldManager->disturbEnvironment(playerBounds, playerWrapper->getVelocity().x);
        }

        float trunkCenter = 0.f;
        float trunkTop = 0.f;
        uint64_t tChunk = 0;
        int tVine = -1, tSeg = -1;
        
        bool touchingTrunk = worldManager->checkTrunkCollision(playerBounds, trunkCenter, trunkTop);
        
        // Disallow climbing up trunks if interacting
        if (touchingTrunk) {
            if (!interactionManager.isInteracting() && (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) && 
                playerWrapper->getState() != ApeState::ClimbingTrunk && 
                playerWrapper->getState() != ApeState::HangingBranch) {
                
                if (playerWrapper->getPosition().y >= trunkTop - 10.f) {
                    playerWrapper->setState(ApeState::ClimbingTrunk);
                    playerWrapper->setPosition(trunkCenter - (playerBounds.width / 2.f), playerWrapper->getPosition().y);
                    playerWrapper->setVelocity(0.f, 0.f);
                    isDroppingToHang = false;
                }
            }
            
            if (playerWrapper->getState() == ApeState::ClimbingTrunk) {
                if (playerWrapper->getPosition().y < trunkTop) {
                    playerWrapper->setPosition(playerWrapper->getPosition().x, trunkTop);
                    if (playerWrapper->getVelocity().y < 0.f) {
                        playerWrapper->setVelocity(0.f, 0.f);
                    }
                }
            }
        } else if (playerWrapper->getState() == ApeState::ClimbingTrunk) {
            playerWrapper->setState(ApeState::Airborne);
        }
        
        if (grabbedVine != -1) { 
            if (worldManager->getVineSegmentCount(grabbedChunk, grabbedVine) == 0) {
                grabbedVine = -1;
                playerWrapper->setState(ApeState::Airborne);
            } else {
                sf::Vector2f vineVel = worldManager->getVineSegmentVelocity(grabbedChunk, grabbedVine, grabbedSeg, dt);
                
                float maxSwingSpeed = 200.f; 
                float swingPush = 400.f * dt; 
                
                if (!interactionManager.isInteracting()) {
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
                        playerWrapper->setState(ApeState::Airborne);
                        float jumpOffX = std::clamp(vineVel.x * 1.5f, -400.f, 400.f);
                        playerWrapper->setVelocity(jumpOffX, -500.f); 
                        grabbedVine = -1;
                    }
                }
            }
        } 
        else if (!interactionManager.isInteracting() && sf::Keyboard::isKeyPressed(sf::Keyboard::G) && playerWrapper->getState() != ApeState::ClimbingVine) {
            if (worldManager->checkVineCollision(playerBounds, tChunk, tVine, tSeg)) {
                playerWrapper->setState(ApeState::ClimbingVine);
                grabbedChunk = tChunk;
                grabbedVine = tVine;
                isDroppingToHang = false;

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
                float transferForce = std::clamp(playerWrapper->getVelocity().x * 0.02f, -12.f, 12.f);
                worldManager->applyVineForce(grabbedChunk, grabbedVine, grabbedSeg, sf::Vector2f(transferForce, 0.f));
            }
        }

        if (playerWrapper->getState() == ApeState::HangingBranch) {
            float apeW = playerBounds.width;
            float apeCenter = playerWrapper->getPosition().x + (apeW / 2.f);
            
            float leftLimit = activeBranch.left + 5.f; 
            float rightLimit = activeBranch.left + activeBranch.width - 5.f;
            
            if (leftLimit > rightLimit) {
                 leftLimit = rightLimit = activeBranch.left + (activeBranch.width / 2.f);
            }

            if (apeCenter < leftLimit) {
                playerWrapper->setPosition(leftLimit - (apeW / 2.f), playerWrapper->getPosition().y);
                playerWrapper->setVelocity(0.f, 0.f);
            } else if (apeCenter > rightLimit) {
                playerWrapper->setPosition(rightLimit - (apeW / 2.f), playerWrapper->getPosition().y);
                playerWrapper->setVelocity(0.f, 0.f);
            }
            
            playerWrapper->setPosition(playerWrapper->getPosition().x, activeBranch.top + activeBranch.height + 120.f);
        }
        
        profiler.physicsTime = physicsClock.getElapsedTime().asSeconds();

        sf::Clock cameraClock; 
        cameraManager->update(dt, playerWrapper->getPosition(), playerWrapper->getVelocity(), playerWrapper->getState());
        profiler.cameraTime = cameraClock.getElapsedTime().asSeconds();

        if (cinematicTextTimer > 0.f) {
            cinematicTextTimer -= dt;
        }

        float playerX = playerWrapper->getPosition().x;
        int foundKingdomId = -1;
        std::string foundKingdomName = "Wilderness";

        for (auto& pair : simulationManager->getRegistry().getAllVillages()) {
            sim::VillageData& v = pair.second;
            if (playerX >= v.centerX - v.territoryRadius && playerX <= v.centerX + v.territoryRadius) {
                foundKingdomId = v.kingdomId;
                foundKingdomName = v.name; 
                break;
            }
        }

        if (foundKingdomId != currentPlayerKingdomId) {
            if (foundKingdomId != -1) {
                cinematicText = "Entering Kingdom of " + foundKingdomName;
            } else {
                cinematicText = "Leaving Kingdom Boundaries";
            }
            currentPlayerKingdomId = foundKingdomId;
            cinematicTextTimer = 4.0f; 
        }
    }

    sf::FloatRect preloadBounds;
    sf::FloatRect unloadBounds = cameraManager->getUnloadBounds();
    
    if (playerWrapper) {
        preloadBounds = cameraManager->getPreloadBounds(playerWrapper->getVelocity());
    } else {
        preloadBounds = cameraManager->getPreloadBounds(sf::Vector2f(0.f, 0.f));
    }

    if (worldManager) {
        worldManager->updateSway(dt, cameraManager->getViewBounds(), weatherManager->getWindVector());
        worldManager->update(dt, preloadBounds, unloadBounds, profiler);
    }

    if (npcManager) {
        npcManager->update(dt, preloadBounds, unloadBounds, *simulationManager, worldManager.get(), worldClock->getTimeOfDay());
    }

    if (playerWrapper) {
        float playerCenterX = playerWrapper->getBounds().left + (playerWrapper->getBounds().width / 2.f);
        float groundHeight = worldManager->getTerrainHeight(playerCenterX);

        if (playerWrapper->getState() == ApeState::ClimbingVine && grabbedVine != -1) {
            climbTimer -= dt;
            if (climbTimer <= 0.f) {
                // Ignore vine climbing controls if interacting
                if (!interactionManager.isInteracting()) {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                        if (grabbedSeg > 1) {
                            grabbedSeg--;
                            climbTimer = 0.15f;
                        }
                    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                        if (grabbedSeg < worldManager->getVineSegmentCount(grabbedChunk, grabbedVine) - 1) {
                            grabbedSeg++;
                            climbTimer = 0.15f;
                        }
                    }
                }
            }

            sf::Vector2f segPos = worldManager->getVineSegmentPosition(grabbedChunk, grabbedVine, grabbedSeg);
            
            float targetY = segPos.y + 120.f;
            if (targetY + playerWrapper->getBounds().height > groundHeight) {
                targetY = groundHeight - playerWrapper->getBounds().height;
            }
            
            playerWrapper->setPosition(segPos.x - (playerWrapper->getBounds().width / 2.f), targetY);
            
            sf::Vector2f vineVel = worldManager->getVineSegmentVelocity(grabbedChunk, grabbedVine, grabbedSeg, dt);
            float fakeVy = (climbTimer > 0.f) ? 10.f : 0.f;
            playerWrapper->setVelocity(vineVel.x, fakeVy);
        }
    }

    sf::Clock pClock;
    weatherManager->update(dt);
    particleSystem->update(dt, cameraManager->getViewBounds(), weatherManager->getWindVector(), weatherManager->getRainIntensity(), simulationManager->getClock().getTimeOfDay());
    profiler.particleTime = pClock.getElapsedTime().asSeconds();
    
    if (audioManager) audioManager->update(dt, weatherManager->getWindIntensity(), weatherManager->getRainIntensity(), worldClock->getTimeOfDay());
    lightingManager->update(dt, cameraManager->getView(), simulationManager->getClock().getTimeOfDay(), weatherManager->getFogDensity());

    background->update(
        cameraManager->getViewBounds().left + cameraManager->getViewBounds().width / 2.f,
        cameraManager->getViewBounds().top + cameraManager->getViewBounds().height / 2.f,
        cameraManager->getView().getSize(),
        dt
    );

    if (playerWrapper) {
        float playerCenterX = playerWrapper->getBounds().left + (playerWrapper->getBounds().width / 2.f);
        float groundHeight = worldManager->getTerrainHeight(playerCenterX);

        profiler.playerPos = playerWrapper->getPosition();
        profiler.cameraPos = cameraManager->getView().getCenter();
        profiler.cameraTarget = cameraManager->getIdealPosition();
        profiler.groundHeight = groundHeight;
        profiler.verticalVelocity = playerWrapper->getVelocity().y;
        profiler.isGrounded = (playerWrapper->getState() == ApeState::Grounded);
        profiler.currentDt = dt;
        profiler.playerStateInt = static_cast<int>(playerWrapper->getState());
        
        if (playerWrapper->getAnimator()) {
            profiler.animName = playerWrapper->getAnimator()->getCurrentAnimationName();
            profiler.animFrame = playerWrapper->getAnimator()->getCurrentFrame();
            profiler.animTime = playerWrapper->getAnimator()->getCurrentTime();
            profiler.animFPS = playerWrapper->getAnimator()->getFPS();
            profiler.animRect = playerWrapper->getAnimator()->getCurrentRect();
            profiler.animOffset = playerWrapper->getAnimator()->getCurrentOffset();
            profiler.spriteScale = playerWrapper->getSprite().getScale();
            profiler.spritePos = playerWrapper->getSprite().getPosition();
            profiler.spriteOrigin = playerWrapper->getSprite().getOrigin();
        }
    }

    if (debugOverlay->getVisible()) {
        ChunkManager* cm = worldManager->getChunkManager();
        float pX = playerWrapper ? playerWrapper->getPosition().x : transitionTarget.x;
        float pY = playerWrapper ? playerWrapper->getPosition().y : transitionTarget.y;
        std::string regionName = Biome::getProperties(cm->getCurrentRegion(pX)).name;
        debugOverlay->updateInfo(dt, cm->getCurrentChunkIndex(), pX, pY, activeSeed, regionName, profiler);

        debugOverlay->updateSimStats(
            static_cast<int>(simulationManager->getRegistry().getAllApes().size()),
            npcManager->getLoadedNPCCount(),
            profiler.chunksLoaded,
            simulationManager->getClock().getTotalTicks(),
            simulationManager->getClock().getHours(),
            simulationManager->getClock().getMinutes(),
            simulationManager->getClock().getDays(),
            static_cast<int>(simulationManager->getRegistry().getSeason()),
            simulationManager->getRegistry().getYear(),
            static_cast<int>(simulationManager->getRegistry().getAllEvents().size())
        );

        sim::ApeData* cData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
        if (cData) {
            sim::DynastyData* d = simulationManager->getRegistry().getDynasty(cData->dynastyId);
            std::string dynName = d ? d->name : "None";
            sim::EntityID currHeir = sim::SuccessionManager::findNextHeir(simulationManager->getRegistry(), cData->id);
            
            int living = 0;
            if (d) {
                for(auto m : d->members) {
                    sim::ApeData* a = simulationManager->getRegistry().getApe(m);
                    if (a && a->alive) living++;
                }
            }
            debugOverlay->updateDynastyStats(cData->name, cData->age, cData->health, dynName, cData->id, currHeir, living);

            sim::VillageData* v = simulationManager->getRegistry().getVillage(cData->villageId);
            if (v && debugOverlay->getShowVillageDebug()) {
                int idle = 0, work = 0, builders = 0, sleep = 0;
                for (auto id : v->members) {
                    sim::ApeData* a = simulationManager->getRegistry().getApe(id);
                    if (a) {
                        if (a->currentJob == sim::Job::Sleep) sleep++;
                        else if (a->currentJob == sim::Job::Builder) builders++;
                        else if (a->currentJob == sim::Job::Idle || a->currentJob == sim::Job::Wander || a->currentJob == sim::Job::Socialize) idle++;
                        else work++;
                    }
                }
                int totalTools = v->toolsAxe + v->toolsPick + v->toolsSpear + v->toolsTorch + v->toolsBasket + v->toolsRope;
                debugOverlay->updateVillageStats(
                    v->name,
                    static_cast<int>(v->members.size()),
                    v->food,
                    v->wood,
                    v->stone,
                    idle,
                    work,
                    builders,
                    sleep,
                    static_cast<int>(v->constructionQueue.size()),
                    static_cast<int>(v->knownVillages.size()),
                    v->territoryRadius,
                    totalTools,
                    v->isMigrating
                );
                std::string recentHistory = "";
                const auto& history = simulationManager->getRegistry().getHistory();
                int startIdx = std::max(0, static_cast<int>(history.size()) - 5);
                for (size_t i = startIdx; i < history.size(); ++i) {
                    recentHistory += "Y" + std::to_string(history[i].year) + " D" + std::to_string(history[i].day) + ": " + history[i].description + "\n";
                }
                debugOverlay->updateHistory(recentHistory);
            }
            
            if (debugOverlay->getShowKingdomDebug()) {
                if (cData->currentKingdom != 0) {
                    sim::KingdomData* kd = simulationManager->getRegistry().getKingdom(cData->currentKingdom);
                    if (kd) {
                        std::string kName = "Unknown";
                        std::string dName = "Unknown";
                        sim::ApeData* king = simulationManager->getRegistry().getApe(kd->currentKingId);
                        if (king) kName = king->name;
                        sim::DynastyData* kDyn = simulationManager->getRegistry().getDynasty(kd->leaderDynastyId);
                        if (kDyn) dName = kDyn->name;
                        
                        int totTreasury = kd->treasuryFood + kd->treasuryWood + kd->treasuryStone + kd->treasuryTools;
                        
                        debugOverlay->updateKingdomStats(
                            kd->name,
                            kName,
                            dName,
                            static_cast<int>(kd->controlledVillages.size()),
                            kd->population,
                            totTreasury,
                            kd->influence,
                            kd->militaryStrength,
                            static_cast<int>(kd->knownKingdoms.size())
                        );
                    }
                }
            }

            if (debugOverlay->getShowWarfareDebug()) {
                std::string warInfo = "--- WARFARE OVERLAY (N) ---\n";
                for (const auto& pair : simulationManager->getRegistry().getAllKingdoms()) {
                    warInfo += "Kingdom: " + pair.second.name + "\n";
                    warInfo += "Territory: [" + std::to_string(static_cast<int>(pair.second.territoryMinX)) + " to " + std::to_string(static_cast<int>(pair.second.territoryMaxX)) + "]\n";
                    warInfo += "Active Armies: " + std::to_string(pair.second.activeArmies.size()) + "\n";
                    for (const auto& tenPair : pair.second.borderTension) {
                        sim::KingdomData* ok = simulationManager->getRegistry().getKingdom(tenPair.first);
                        if (ok) {
                            warInfo += "  Tension w/ " + ok->name + ": " + std::to_string(static_cast<int>(tenPair.second)) + "\n";
                        }
                    }
                }
                warInfo += "\n--- ARMIES ---\n";
                for (const auto& pair : simulationManager->getRegistry().getAllArmies()) {
                    warInfo += "Army " + std::to_string(pair.second.id % 100) + " | Size: " + std::to_string(pair.second.members.size());
                    warInfo += " | Objective: " + std::to_string(static_cast<int>(pair.second.objective)) + "\n";
                }
                debugOverlay->updateWarfareStats(warInfo);
            }
        }
    }
    
    profiler.updateTime = updateClock.getElapsedTime().asSeconds();
}

void PlayState::draw(sf::RenderWindow& window) {
    sf::Clock renderClock;

    window.setView(cameraManager->getView());
    window.clear();

    background->draw(window);
    
    if (lightingManager) lightingManager->drawFog(window);

    if (worldManager) {
        sf::FloatRect preB = playerWrapper ? cameraManager->getPreloadBounds(playerWrapper->getVelocity()) : cameraManager->getPreloadBounds(sf::Vector2f(0.f, 0.f));
        worldManager->drawBackground(window, cameraManager->getViewBounds(), debugOverlay->getShowFoliage(), profiler, game->getAssetManager().getTexture("tileset"));
        
        if (structureManager) {
            structureManager->draw(window, simulationManager->getRegistry(), worldManager.get(), cameraManager->getViewBounds());
        }

        if (debugOverlay && debugOverlay->getShowWarfareDebug()) {
            for(auto& p : simulationManager->getRegistry().getAllKingdoms()) {
                if (p.second.territoryMaxX > p.second.territoryMinX) {
                    sf::RectangleShape rect(sf::Vector2f(p.second.territoryMaxX - p.second.territoryMinX, 2000.f));
                    sf::Color c = p.second.color;
                    c.a = 40; 
                    rect.setFillColor(c);
                    rect.setPosition(p.second.territoryMinX, -1000.f);
                    window.draw(rect);
                }
            }
        }

        worldManager->drawTerritoryMarkers(window, simulationManager->getRegistry(), cameraManager->getViewBounds());
        
        worldManager->drawGeometry(window, cameraManager->getViewBounds(), profiler);
        for(auto& p : simulationManager->getRegistry().getAllKingdoms()) {
            if (p.second.territoryMinX != 0.f) {
                float yLeft = worldManager->getTerrainHeight(p.second.territoryMinX);
                sf::RectangleShape leftTotem(sf::Vector2f(12.f, 100.f));
                leftTotem.setOrigin(6.f, 100.f);
                leftTotem.setPosition(p.second.territoryMinX, yLeft);
                leftTotem.setFillColor(sf::Color(70, 45, 25));
                leftTotem.setOutlineColor(sf::Color(30, 15, 5));
                leftTotem.setOutlineThickness(2.f);
                window.draw(leftTotem);
            }
            if (p.second.territoryMaxX != 0.f) {
                float yRight = worldManager->getTerrainHeight(p.second.territoryMaxX);
                sf::RectangleShape rightTotem(sf::Vector2f(12.f, 100.f));
                rightTotem.setOrigin(6.f, 100.f);
                rightTotem.setPosition(p.second.territoryMaxX, yRight);
                rightTotem.setFillColor(sf::Color(70, 45, 25));
                rightTotem.setOutlineColor(sf::Color(30, 15, 5));
                rightTotem.setOutlineThickness(2.f);
                window.draw(rightTotem);
            }
        }
        if (particleSystem) particleSystem->draw(window);

        if (debugOverlay) {
            worldManager->drawDebug(window, 
                cameraManager->getViewBounds(), 
                preB, 
                cameraManager->getUnloadBounds(),
                debugOverlay.get());
        }
    }
    
    if (npcManager) npcManager->draw(window);
    if (playerWrapper) playerWrapper->draw(window);

    if (debugOverlay && debugOverlay->getShowVillageDebug()) {
        for(auto& p : simulationManager->getRegistry().getAllVillages()) {
            sf::CircleShape terr(p.second.territoryRadius);
            terr.setFillColor(sf::Color(0, 255, 0, 30));
            terr.setOrigin(p.second.territoryRadius, p.second.territoryRadius);
            terr.setPosition(p.second.centerX, p.second.centerY);
            window.draw(terr);
        }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)) {
        sim::EntityID heirId = sim::SuccessionManager::findNextHeir(simulationManager->getRegistry(), simulationManager->getControlledApe());
        if (heirId != 0) {
            sim::ApeData* heirData = simulationManager->getRegistry().getApe(heirId);
            if (heirData) {
                sf::RectangleShape highlight(sf::Vector2f(40.f, 40.f));
                highlight.setFillColor(sf::Color::Transparent);
                highlight.setOutlineColor(sf::Color::Yellow);
                highlight.setOutlineThickness(2.f);
                highlight.setPosition(heirData->worldX, heirData->worldY);
                window.draw(highlight);
            }
        }
    }
    
    window.setView(window.getDefaultView());
    
    if (cinematicTextTimer > 0.f) {
        float alpha = 255.f;
        if (cinematicTextTimer > 3.0f) alpha = (4.0f - cinematicTextTimer) * 255.f; 
        else if (cinematicTextTimer < 1.0f) alpha = cinematicTextTimer * 255.f;     

        sf::Text transitionText(cinematicText, cinematicFont, 36);
        
        sf::FloatRect textRect = transitionText.getLocalBounds();
        transitionText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
        transitionText.setPosition(window.getSize().x / 2.0f, window.getSize().y / 4.0f);
        
        transitionText.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));
        transitionText.setOutlineColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(alpha)));
        transitionText.setOutlineThickness(2.f);

        window.draw(transitionText);
    }

    if (lightingManager) lightingManager->drawAmbient(window);

    window.setView(cameraManager->getView());

    interactionManager.draw(window);

    if (debugOverlay) debugOverlay->draw(window);
    
    profiler.renderTime = renderClock.getElapsedTime().asSeconds();
}

void PlayState::refreshInteractionTargets() {
    interactionManager.clearTargets();

    for (const auto& pair : simulationManager->getRegistry().getAllVillages()) {
        const sim::VillageData& v = pair.second;
        float groundHeight = worldManager->getTerrainHeight(v.centerX);
        
        interactionManager.registerTarget(std::make_shared<BonfireInteractionTarget>(
            v.id, simulationManager->getRegistry(), v.centerX, groundHeight - 50.f, audioManager.get(), particleSystem.get()
        ));

        interactionManager.registerTarget(std::make_shared<VillageCenterInteractionTarget>(
            v.id, simulationManager->getRegistry(), v.centerX - 150.f, groundHeight - 50.f, audioManager.get()
        ));
        
        interactionManager.registerTarget(std::make_shared<StorageHutInteractionTarget>(
            v.id, simulationManager->getRegistry(), v.centerX + 250.f, groundHeight - 50.f
        ));
    }

    for (const auto& pair : simulationManager->getRegistry().getAllKingdoms()) {
        const sim::KingdomData& k = pair.second;
        
        if (k.territoryMinX != 0.f) {
            interactionManager.registerTarget(std::make_shared<BorderTotemInteractionTarget>(
                k.id, simulationManager->getRegistry(), worldManager.get(), true
            ));
        }
        if (k.territoryMaxX != 0.f) {
            interactionManager.registerTarget(std::make_shared<BorderTotemInteractionTarget>(
                k.id, simulationManager->getRegistry(), worldManager.get(), false
            ));
        }

        if (k.currentKingId != 0) {
            interactionManager.registerTarget(std::make_shared<KingInteractionTarget>(
                k.currentKingId, 
                simulationManager->getRegistry(),
                simulationManager->getControlledApe()
            ));
        }
    }
}