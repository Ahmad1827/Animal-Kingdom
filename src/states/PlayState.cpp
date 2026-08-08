#include "states/PlayState.h"
#include "core/Game.h"
#include "world/Biome.h"
#include "simulation/PopulationGenerator.h"
#include "simulation/SuccessionManager.h"
#include "simulation/KingdomManager.h"
#include "simulation/WarfareManager.h"
#include "world/targets/BonfireInteractionTarget.hpp"
#include "world/targets/VillageCenterInteractionTarget.hpp"
#include "world/targets/StorageHutInteractionTarget.hpp"
#include "world/targets/KingInteractionTarget.hpp"
#include "world/targets/BorderTotemInteractionTarget.hpp"
#include "world/targets/DiplomaticMeetingInteractionTarget.hpp"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>
#include <algorithm>

PlayState::PlayState(Game* game) : game(game), isTransitioning(false), transitionTimer(0.f), f3PressedLastFrame(false), f4PressedLastFrame(false), f5PressedLastFrame(false), f6PressedLastFrame(false), f7PressedLastFrame(false), f8PressedLastFrame(false), f9PressedLastFrame(false), f10PressedLastFrame(false), f11PressedLastFrame(false) {}

void PlayState::init() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    activeSeed = std::rand();
    background = std::make_unique<Background>(game->getAssetManager());
    worldManager = std::make_unique<WorldManager>(activeSeed, game->getAssetManager().getTexture("decors"));
    cameraManager = std::make_unique<CameraManager>(sf::Vector2f(1280.f, 720.f));
    lightingManager = std::make_unique<LightingManager>();
    profilePanelPos = sf::Vector2f(40.f, 85.f);
    currentMapZoom = 6.0f;
    targetMapZoom = 6.0f;
    currentViewport = sf::FloatRect(0.70f, 0.05f, 0.28f, 0.28f);
    targetViewport = currentViewport;
    
    mapView.setSize(1280.f * currentMapZoom, 720.f * currentMapZoom);
    mapView.setViewport(currentViewport);
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
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Tab) {
        if (mapMode == MapMode::Hidden) {
            mapMode = MapMode::Mini;
            targetViewport = sf::FloatRect(0.70f, 0.05f, 0.28f, 0.28f);
            targetMapZoom = 6.0f;
            isMapDetached = false;
        } else if (mapMode == MapMode::Mini) {
            mapMode = MapMode::Expanded;
            targetViewport = sf::FloatRect(0.15f, 0.10f, 0.70f, 0.75f); // Large centered map
            targetMapZoom = 12.0f; // Zoom out to see the realm
        } else {
            mapMode = MapMode::Hidden;
        }
        return; 
    }

    if (mapMode != MapMode::Hidden) {
        // Handle "Close" logic for Map Profiles (Escape key always backs out of the deepest view)
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            if (isInspectingCharacter) {
                isInspectingCharacter = false; // Back to Village/Kingdom Profile
                return;
            } else if (selectedVillageId != 0 || selectedKingdomId != 0) {
                selectedVillageId = 0; // Clear selection to close profile
                selectedKingdomId = 0;
                return;
            } else {
                mapMode = MapMode::Hidden; // Close map
                return;
            }
        }
        
        // Calculate the map's pixel boundaries
        sf::Vector2f winSize(game->getWindow().getSize().x, game->getWindow().getSize().y);
        sf::FloatRect vpPixels(
            currentViewport.left * winSize.x, currentViewport.top * winSize.y,
            currentViewport.width * winSize.x, currentViewport.height * winSize.y
        );

        // --- PROFILE DRAG & CLICK LOGIC ---
        sf::FloatRect profileRect(0, 0, 0, 0);
        bool profileOpen = false;
        if (isInspectingCharacter) { profileRect = {profilePanelPos.x, profilePanelPos.y, 320.f, 500.f}; profileOpen = true; }
        else if (selectedKingdomId != 0) { profileRect = {profilePanelPos.x, profilePanelPos.y, 380.f, 550.f}; profileOpen = true; }
        else if (selectedVillageId != 0) { profileRect = {profilePanelPos.x, profilePanelPos.y, 340.f, 500.f}; profileOpen = true; }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            // Check if clicking inside the floating profile panel
            if (profileOpen && profileRect.contains(event.mouseButton.x, event.mouseButton.y)) {
                isDraggingProfile = true;
                lastMousePos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
                dragStartMousePos = lastMousePos;
                return; // Consume
            }
            // Check if clicking the map
            if (vpPixels.contains(event.mouseButton.x, event.mouseButton.y)) {
                isDraggingMap = true;
                isMapDetached = true;
                lastMousePos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
                dragStartMousePos = lastMousePos;
                return; // Consume
            }
        }
        if (event.type == sf::Event::MouseMoved) {
            if (isDraggingProfile) {
                sf::Vector2i newPos(event.mouseMove.x, event.mouseMove.y);
                profilePanelPos.x += (newPos.x - lastMousePos.x);
                profilePanelPos.y += (newPos.y - lastMousePos.y);
                lastMousePos = newPos;
                return;
            }
            if (isDraggingMap) {
                sf::Vector2i newPos(event.mouseMove.x, event.mouseMove.y);
                sf::Vector2f delta = game->getWindow().mapPixelToCoords(lastMousePos, mapView) - game->getWindow().mapPixelToCoords(newPos, mapView);
                mapCenter += delta;
                lastMousePos = newPos;
                return;
            }
        }
        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            if (isDraggingProfile) {
                isDraggingProfile = false;
                int dx = event.mouseButton.x - dragStartMousePos.x;
                int dy = event.mouseButton.y - dragStartMousePos.y;
                if (dx * dx + dy * dy < 25) {
                    // Clicked inside the profile (check button bounds dynamically)
                    float btnTop = profilePanelPos.y + profileRect.height - 80.f;
                    float btnBottom = profilePanelPos.y + profileRect.height - 40.f;
                    float btnLeft = profilePanelPos.x + profileRect.width / 2.f - 100.f;
                    float btnRight = profilePanelPos.x + profileRect.width / 2.f + 100.f;
                    
                    if (event.mouseButton.x >= btnLeft && event.mouseButton.x <= btnRight &&
                        event.mouseButton.y >= btnTop && event.mouseButton.y <= btnBottom) {
                        if (isInspectingCharacter) isInspectingCharacter = false; // "Back"
                        else isInspectingCharacter = true; // "View Leader"
                    }
                }
                return;
            }
            if (isDraggingMap) {
                isDraggingMap = false;
                int dx = event.mouseButton.x - dragStartMousePos.x;
                int dy = event.mouseButton.y - dragStartMousePos.y;
                if (dx * dx + dy * dy < 25) {
                    handleMapClick(game->getWindow().mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), mapView));
                }
                return;
            }
        }
        if (event.type == sf::Event::MouseWheelScrolled) {
            if (vpPixels.contains(event.mouseWheelScroll.x, event.mouseWheelScroll.y)) {
                targetMapZoom -= event.mouseWheelScroll.delta * 1.5f;
                targetMapZoom = std::clamp(targetMapZoom, 2.0f, 40.0f);
                isMapDetached = true;
                return;
            }
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::C) {
            isMapDetached = false;
            return;
        }
    }

    if (isDialogueActive) {
        if (event.type == sf::Event::KeyPressed) {
            // If inspecting, ANY interaction key returns to the conversation seamlessly
            if (isInspectingCharacter) {
                if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::E || event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                    isInspectingCharacter = false;
                }
                return; // Consume event so we don't trigger anything else
            }

            if (event.key.code == sf::Keyboard::Escape) {
                endDiplomaticDialogue();
            } else if (event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up) {
                if (!dialogueOptions.empty()) {
                    dialogueSelectedIndex = (dialogueSelectedIndex - 1 + dialogueOptions.size()) % dialogueOptions.size();
                }
            } else if (event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::Down) {
                if (!dialogueOptions.empty()) {
                    dialogueSelectedIndex = (dialogueSelectedIndex + 1) % dialogueOptions.size();
                }
            } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::E || event.key.code == sf::Keyboard::Space) {
                if (!dialogueOptions.empty() && dialogueSelectedIndex < static_cast<int>(dialogueOptions.size())) {
                    dialogueOptions[dialogueSelectedIndex].action();
                }
            }
        }
        return; // Consume ALL events while locked in dialogue
    }

    // 2. Safely cancel the WAIT state if ESC is pressed BEFORE they arrive
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        sim::ApeData* pWaitCheck = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
        if (pWaitCheck && pWaitCheck->isWaitingForAudience) {
            endDiplomaticDialogue(); // Reuses our clean exit function
            interactionManager.clearTargets();
            refreshInteractionTargets(); 
            return;
        }
    }

    // Normal interaction manager handling
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

    sim::ApeData* pDataCheck = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
    bool isCinematicWait = false;
    if (pDataCheck && pDataCheck->isWaitingForAudience) {
        sim::ApeData* repCheck = simulationManager->getRegistry().getApe(pDataCheck->summonedRepId);
        if (repCheck) {
            float dist = std::abs(pDataCheck->meetingX - repCheck->worldX);
            if (dist > 150.f) {
                isCinematicWait = true;
            } else if (!isDialogueActive) {
                // REPRESENTATIVE ARRIVED! Instantly hand off to the dialogue state!
                startDiplomaticDialogue(repCheck->id);
            }
        }
    }
    
    if (!interactionManager.isInteracting() && !isCinematicWait && !isDialogueActive) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) cameraManager->setZoom(0.5f);
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) cameraManager->setZoom(2.0f);
        else cameraManager->setZoom(1.35f);
    } else if (isCinematicWait || isDialogueActive) {
        // Enforce cinematic zoom during BOTH the waiting phase AND the active dialogue phase
        cameraManager->setZoom(0.8f); 
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
        interactionManager.update(dt, playerWrapper->getPosition(), *cameraManager);
        playerWrapper->update(dt);
        
        if (interactionManager.isInteracting() || isCinematicWait || isDialogueActive) {
            playerWrapper->setVelocity(0.f, playerWrapper->getVelocity().y);
        }

        // Minimap actively tracks the player as they move
        // --- MAP ANIMATION AND LERPING ---
        if (mapMode != MapMode::Hidden) {
            float lerp = 1.0f - std::exp(-8.0f * dt); // Smooth interpolation
            
            // 1. Animate the UI Viewport expanding/shrinking
            currentViewport.left += (targetViewport.left - currentViewport.left) * lerp;
            currentViewport.top += (targetViewport.top - currentViewport.top) * lerp;
            currentViewport.width += (targetViewport.width - currentViewport.width) * lerp;
            currentViewport.height += (targetViewport.height - currentViewport.height) * lerp;
            mapView.setViewport(currentViewport);

            // 2. Animate the Zoom level
            currentMapZoom += (targetMapZoom - currentMapZoom) * lerp;
            mapView.setSize(1280.f * currentMapZoom, 720.f * currentMapZoom);

            // 3. Track player ONLY if the map is not manually detached
            if (!isMapDetached && playerWrapper) {
                sf::Vector2f targetCenter(playerWrapper->getPosition().x, playerWrapper->getPosition().y - 1000.f);
                mapCenter += (targetCenter - mapCenter) * lerp;
            }
            mapView.setCenter(mapCenter);
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
            // CHECK AGAINST STATIC BOUNDARIES
            if (playerX >= v.borderMinX && playerX <= v.borderMaxX) {
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
            float yLeft = worldManager->getTerrainHeight(p.second.territoryMinX);
            sf::RectangleShape leftTotem(sf::Vector2f(16.f, 150.f));
            leftTotem.setOrigin(8.f, 150.f);
            leftTotem.setPosition(p.second.territoryMinX, yLeft);
            leftTotem.setFillColor(sf::Color(60, 40, 20));
            leftTotem.setOutlineColor(sf::Color::Black);
            leftTotem.setOutlineThickness(2.f);
            window.draw(leftTotem);

            float yRight = worldManager->getTerrainHeight(p.second.territoryMaxX);
            sf::RectangleShape rightTotem(sf::Vector2f(16.f, 150.f));
            rightTotem.setOrigin(8.f, 150.f);
            rightTotem.setPosition(p.second.territoryMaxX, yRight);
            rightTotem.setFillColor(sf::Color(60, 40, 20));
            rightTotem.setOutlineColor(sf::Color::Black);
            rightTotem.setOutlineThickness(2.f);
            window.draw(rightTotem);
        }

        // --- DRAW UNIFIED DIPLOMATIC MEETING GROUNDS ---
        struct Polity { sim::EntityID id; bool isKingdom; float minX; float maxX; };
        std::vector<Polity> polities;
        for (const auto& pair : simulationManager->getRegistry().getAllVillages()) {
            if (pair.second.kingdomId == 0) {
                // MEETING POINT NOW RELIES EXCLUSIVELY ON STATIC BORDERS
                polities.push_back({pair.first, false, pair.second.borderMinX, pair.second.borderMaxX});
            }
        }
        for (const auto& pair : simulationManager->getRegistry().getAllKingdoms()) {
            if (pair.second.territoryMaxX != 0.f) {
                polities.push_back({pair.first, true, pair.second.territoryMinX, pair.second.territoryMaxX});
            }
        }
        for (size_t i = 0; i < polities.size(); ++i) {
            for (size_t j = i + 1; j < polities.size(); ++j) {
                const Polity& p1 = polities[i];
                const Polity& p2 = polities[j];
                
                float gap = 0.f;
                float midX = 0.f;

                if (p1.maxX < p2.minX) {
                    gap = p2.minX - p1.maxX;
                    midX = p1.maxX + (gap / 2.f);
                } else if (p2.maxX < p1.minX) {
                    gap = p1.minX - p2.maxX;
                    midX = p2.maxX + (gap / 2.f);
                } else {
                    gap = 0.f;
                    float overlapStart = std::max(p1.minX, p2.minX);
                    float overlapEnd = std::min(p1.maxX, p2.maxX);
                    midX = overlapStart + ((overlapEnd - overlapStart) / 2.f);
                }

                if (gap >= 0.f && gap <= 4000.f) {
                    float midY = worldManager->getTerrainHeight(midX);
                    
                    sf::Color color1 = sf::Color(40, 140, 40); // Default Tribal Green
                    if (p1.isKingdom) {
                        sim::KingdomData* k1 = simulationManager->getRegistry().getKingdom(p1.id);
                        if (k1) color1 = k1->color;
                    }
                    
                    sf::Color color2 = sf::Color(40, 140, 40); // Default Tribal Green
                    if (p2.isKingdom) {
                        sim::KingdomData* k2 = simulationManager->getRegistry().getKingdom(p2.id);
                        if (k2) color2 = k2->color;
                    }
                    
                    // Central Stone Fire Pit
                    sf::RectangleShape firePit(sf::Vector2f(60.f, 15.f));
                    firePit.setOrigin(30.f, 15.f);
                    firePit.setPosition(midX, midY);
                    firePit.setFillColor(sf::Color(100, 100, 100));
                    firePit.setOutlineColor(sf::Color::Black);
                    firePit.setOutlineThickness(2.f);
                    window.draw(firePit);
                    
                    // Fire (Primitive triangles)
                    sf::ConvexShape flameOuter(3);
                    flameOuter.setPoint(0, sf::Vector2f(0.f, -30.f));
                    flameOuter.setPoint(1, sf::Vector2f(15.f, 0.f));
                    flameOuter.setPoint(2, sf::Vector2f(-15.f, 0.f));
                    flameOuter.setPosition(midX, midY - 15.f);
                    flameOuter.setFillColor(sf::Color(220, 80, 20));
                    window.draw(flameOuter);
                    
                    sf::ConvexShape flameInner(3);
                    flameInner.setPoint(0, sf::Vector2f(0.f, -15.f));
                    flameInner.setPoint(1, sf::Vector2f(8.f, 0.f));
                    flameInner.setPoint(2, sf::Vector2f(-8.f, 0.f));
                    flameInner.setPosition(midX, midY - 15.f);
                    flameInner.setFillColor(sf::Color(240, 200, 40));
                    window.draw(flameInner);

                    // Left Banner (Entity 1)
                    sf::RectangleShape leftPole(sf::Vector2f(4.f, 80.f));
                    leftPole.setOrigin(2.f, 80.f);
                    leftPole.setPosition(midX - 60.f, midY);
                    leftPole.setFillColor(sf::Color(90, 60, 40));
                    window.draw(leftPole);
                    
                    sf::RectangleShape leftFlag(sf::Vector2f(30.f, 40.f));
                    leftFlag.setOrigin(30.f, 0.f); // Hangs towards the fire
                    leftFlag.setPosition(midX - 60.f, midY - 75.f);
                    leftFlag.setFillColor(color1);
                    leftFlag.setOutlineColor(sf::Color::Black);
                    leftFlag.setOutlineThickness(1.f);
                    window.draw(leftFlag);

                    // Right Banner (Entity 2)
                    sf::RectangleShape rightPole(sf::Vector2f(4.f, 80.f));
                    rightPole.setOrigin(2.f, 80.f);
                    rightPole.setPosition(midX + 60.f, midY);
                    rightPole.setFillColor(sf::Color(90, 60, 40));
                    window.draw(rightPole);

                    sf::RectangleShape rightFlag(sf::Vector2f(30.f, 40.f));
                    rightFlag.setOrigin(0.f, 0.f); // Hangs towards the fire
                    rightFlag.setPosition(midX + 60.f, midY - 75.f);
                    rightFlag.setFillColor(color2);
                    rightFlag.setOutlineColor(sf::Color::Black);
                    rightFlag.setOutlineThickness(1.f);
                    window.draw(rightFlag);
                }
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
            // Read strictly from the fixed physical bounds
            float minX = p.second.borderMinX;
            float maxX = p.second.borderMaxX;
            
            // If it belongs to a kingdom, map tightly to the kingdom's frozen borders
            if (p.second.kingdomId != 0) {
                sim::KingdomData* kData = simulationManager->getRegistry().getKingdom(p.second.kingdomId);
                if (kData && kData->territoryMaxX > kData->territoryMinX) {
                    minX = kData->territoryMinX;
                    maxX = kData->territoryMaxX;
                }
            }
            
            float width = maxX - minX;
            if (width > 0.f) {
                // Static visual matching the authoritative borders exactly
                sf::RectangleShape terr(sf::Vector2f(width, 4000.f));
                terr.setFillColor(sf::Color(0, 255, 0, 30));
                terr.setOrigin(0.f, 2000.f);
                terr.setPosition(minX, p.second.centerY);
                window.draw(terr);
            }
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
    // --- CINEMATIC DIPLOMATIC ARRIVAL UI ---
    sim::ApeData* pDataHUD = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
    bool isCinematicWait = false;
    
    if (pDataHUD && pDataHUD->isWaitingForAudience) {
        sim::ApeData* rep = simulationManager->getRegistry().getApe(pDataHUD->summonedRepId);
        if (rep) {
            float dist = std::abs(rep->worldX - pDataHUD->meetingX);
            isCinematicWait = (dist > 150.0f); 
            
            if (isCinematicWait) {
                // True physical countdown, capped at a minimum of 0
                int secondsLeft = std::max(0, static_cast<int>(dist / 180.0f));

                // A bit more writing, bigger size, and prettier elegant gold formatting
                sf::Text waitText("Awaiting Diplomatic Representative...", cinematicFont, 24);
                sf::FloatRect waitRect = waitText.getLocalBounds();
                waitText.setOrigin(waitRect.left + waitRect.width / 2.0f, waitRect.top + waitRect.height / 2.0f);
                waitText.setPosition(window.getSize().x / 2.0f, window.getSize().y * 0.12f);
                waitText.setFillColor(sf::Color(255, 215, 100, 255)); // Strong, warm gold
                waitText.setOutlineColor(sf::Color(0, 0, 0, 255)); // Solid black outline for readability
                waitText.setOutlineThickness(2.0f);
                window.draw(waitText);

                // Polished countdown text with descriptive wording
                // Polished countdown text with descriptive wording
                std::string timeString = "Arrival in: " + std::to_string(secondsLeft) + " seconds";
                sf::Text timeText(timeString, cinematicFont, 18);
                sf::FloatRect timeRect = timeText.getLocalBounds();
                timeText.setOrigin(timeRect.left + timeRect.width / 2.0f, timeRect.top + timeRect.height / 2.0f);
                timeText.setPosition(window.getSize().x / 2.0f, window.getSize().y * 0.12f + 32.f);
                timeText.setFillColor(sf::Color(220, 230, 240, 255)); // Crisp off-white
                timeText.setOutlineColor(sf::Color(0, 0, 0, 255));
                timeText.setOutlineThickness(2.0f);
                window.draw(timeText);
            }
        }
    }

    // --- DIEGETIC DIALOGUE RENDERING ---
    // --- DIEGETIC DIALOGUE RENDERING ---
    // --- DIEGETIC DIALOGUE RENDERING ---
    if (isDialogueActive) {
        if (isInspectingCharacter) {
            // ONLY draw the profile, hiding the dialogue entirely
            if (pDataHUD && pDataHUD->summonedRepId != 0) {
                drawCharacterProfile(window, pDataHUD->summonedRepId);
                
                // Draw a simple prompt to let the player know how to go back
                sf::Text returnText("[ESC / E] Return to Conversation", cinematicFont, 16);
                returnText.setFillColor(sf::Color(200, 200, 200));
                returnText.setOutlineColor(sf::Color::Black);
                returnText.setOutlineThickness(1.5f);
                sf::FloatRect rBounds = returnText.getLocalBounds();
                returnText.setOrigin(rBounds.left + rBounds.width / 2.f, rBounds.top + rBounds.height / 2.f);
                returnText.setPosition(window.getSize().x / 2.f, window.getSize().y - 50.f);
                window.draw(returnText);
            }
        } else {
            // NORMAL DIALOGUE (No Profile)
            // Speaker Title
            sf::Text speakerText(dialogueSpeakerName, cinematicFont, 24);
            speakerText.setFillColor(sf::Color(255, 215, 100)); // Gold
            speakerText.setOutlineColor(sf::Color::Black);
            speakerText.setOutlineThickness(2.f);
            sf::FloatRect sRect = speakerText.getLocalBounds();
            speakerText.setOrigin(sRect.left + sRect.width / 2.0f, sRect.top + sRect.height / 2.0f);
            speakerText.setPosition(window.getSize().x / 2.0f, window.getSize().y * 0.12f);
            window.draw(speakerText);

            // Body Text
            sf::Text bodyText(dialogueText, cinematicFont, 20);
            bodyText.setFillColor(sf::Color::White);
            bodyText.setOutlineColor(sf::Color::Black);
            bodyText.setOutlineThickness(2.f);
            sf::FloatRect bRect = bodyText.getLocalBounds();
            bodyText.setOrigin(bRect.left + bRect.width / 2.0f, bRect.top + bRect.height / 2.0f);
            bodyText.setPosition(window.getSize().x / 2.0f, window.getSize().y * 0.12f + 40.f);
            window.draw(bodyText);

            // Options Text
            float optionsStartY = window.getSize().y * 0.12f + 110.f;
            for (size_t i = 0; i < dialogueOptions.size(); ++i) {
                sf::Text optText("", cinematicFont, 18);
                if (static_cast<int>(i) == dialogueSelectedIndex) {
                    optText.setFillColor(sf::Color(255, 255, 150)); // Highlighted yellow
                    optText.setString("> " + dialogueOptions[i].text + " <");
                } else {
                    optText.setFillColor(sf::Color(180, 180, 180)); // Dimmed
                    optText.setString(dialogueOptions[i].text);
                }
                optText.setOutlineColor(sf::Color::Black);
                optText.setOutlineThickness(1.5f);
                sf::FloatRect oRect = optText.getLocalBounds();
                optText.setOrigin(oRect.left + oRect.width / 2.0f, oRect.top + oRect.height / 2.0f);
                optText.setPosition(window.getSize().x / 2.0f, optionsStartY + (i * 30.f));
                window.draw(optText);
            }
        } // Close the 'else' block for normal dialogue
    }

    if (lightingManager) lightingManager->drawAmbient(window);

    window.setView(cameraManager->getView());

    if (!isCinematicWait) {
        interactionManager.draw(window);
    }

    if (mapMode != MapMode::Hidden) {
        drawWorldMap(window);
        
        // --- FIX: Switch to screen-space (HUD) view so profiles stay fixed while panning the map ---
        sf::View prevView = window.getView();
        window.setView(window.getDefaultView());

        // Render Information Panels on top of the Map
        if (isInspectingCharacter) {
            // Reusing the character profile if inspecting a leader from the map
            sim::EntityID targetApe = 0;
            if (selectedVillageId != 0) {
                sim::VillageData* v = simulationManager->getRegistry().getVillage(selectedVillageId);
                if (v) targetApe = v->leaderId;
            } else if (selectedKingdomId != 0) {
                sim::KingdomData* k = simulationManager->getRegistry().getKingdom(selectedKingdomId);
                if (k) targetApe = k->currentKingId;
            }
            if (targetApe != 0) drawCharacterProfile(window, targetApe);
        } else if (selectedVillageId != 0) {
            drawVillageProfile(window, selectedVillageId);
        } else if (selectedKingdomId != 0) {
            drawKingdomProfile(window, selectedKingdomId);
        }
        
        // Restore previous map view
        window.setView(prevView);
    }

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

    // --- UNIFIED DIPLOMATIC MEETING GROUNDS ---
    struct Polity { sim::EntityID id; bool isKingdom; float minX; float maxX; };
    std::vector<Polity> polities;
    
    for (const auto& pair : simulationManager->getRegistry().getAllVillages()) {
        if (pair.second.kingdomId == 0) {
            // INTERACTION TARGET NOW RELIES EXCLUSIVELY ON STATIC BORDERS
            polities.push_back({pair.first, false, pair.second.borderMinX, pair.second.borderMaxX});
        }
    }
    
    for (const auto& pair : simulationManager->getRegistry().getAllKingdoms()) {
        if (pair.second.territoryMaxX != 0.f) {
            polities.push_back({pair.first, true, pair.second.territoryMinX, pair.second.territoryMaxX});
        }
    }

    for (size_t i = 0; i < polities.size(); ++i) {
        for (size_t j = i + 1; j < polities.size(); ++j) {
            const Polity& p1 = polities[i];
            const Polity& p2 = polities[j];
            
            float gap = 0.f;
            float midX = 0.f;

            if (p1.maxX < p2.minX) {
                gap = p2.minX - p1.maxX;
                midX = p1.maxX + (gap / 2.f);
            } else if (p2.maxX < p1.minX) {
                gap = p1.minX - p2.maxX;
                midX = p2.maxX + (gap / 2.f);
            } else {
                gap = 0.f;
                float overlapStart = std::max(p1.minX, p2.minX);
                float overlapEnd = std::min(p1.maxX, p2.maxX);
                midX = overlapStart + ((overlapEnd - overlapStart) / 2.f);
            }

            if (gap >= 0.f && gap <= 4000.f) {
                float midY = worldManager->getTerrainHeight(midX);
                interactionManager.registerTarget(std::make_shared<DiplomaticMeetingInteractionTarget>(
                    p1.id, p1.isKingdom, p2.id, p2.isKingdom, midX, midY, simulationManager->getRegistry(), simulationManager->getControlledApe()
                ));
            }
        }
    }
}

void PlayState::endDiplomaticDialogue() {
    isDialogueActive = false;
    isInspectingCharacter = false; // Reset inspection state for the next meeting
    sim::ApeData* pData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
    if (pData && pData->isWaitingForAudience) {
        pData->isWaitingForAudience = false;
        sim::ApeData* rep = simulationManager->getRegistry().getApe(pData->summonedRepId);
        if (rep) {
            rep->hasTravelDestination = true;
            rep->travelDestinationX = rep->homeX; 
        }
        pData->summonedRepId = 0;
    }
}

void PlayState::startDiplomaticDialogue(sim::EntityID repId) {
    isDialogueActive = true;
    dialogueSelectedIndex = 0;
    dialogueOptions.clear();

    sim::ApeData* player = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
    sim::ApeData* rep = simulationManager->getRegistry().getApe(repId);
    if (!player || !rep) { endDiplomaticDialogue(); return; }

    std::string entityName = "Unknown Land";
    bool isKing = false;
    sim::DiplomacyStatus status = sim::DiplomacyStatus::Neutral;
    
    sim::KingdomID pKID = player->currentKingdom;
    sim::KingdomID rKID = rep->currentKingdom;

    // Retrieve backend simulation data to drive the dialogue
    if (rKID != 0) {
        sim::KingdomData* rK = simulationManager->getRegistry().getKingdom(rKID);
        if (rK) {
            entityName = "Kingdom of " + rK->name;
            if (rK->currentKingId == rep->id) isKing = true;
            if (pKID != 0) {
                sim::KingdomData* pK = simulationManager->getRegistry().getKingdom(pKID);
                if (pK && pK->relations.count(rKID)) {
                    status = pK->relations[rKID];
                }
            }
        }
    } else {
        sim::VillageData* v = simulationManager->getRegistry().getVillage(rep->villageId);
        if (v) entityName = "Village of " + v->name;
    }

    dialogueSpeakerName = (isKing ? "King " : "Representative ") + rep->name + " of " + entityName;

    // Data-driven contextual dialogue
    if (status == sim::DiplomacyStatus::War) {
        dialogueText = "\"Your armies have crossed our borders.\nThere is little left to discuss.\"";
        dialogueOptions.push_back({"Demand surrender.", [this]() { endDiplomaticDialogue(); }});
        dialogueOptions.push_back({"Propose peace.", [this, pKID, rKID]() { 
            sim::WarfareManager::cancelWar(simulationManager->getRegistry(), pKID, rKID);
            endDiplomaticDialogue(); 
        }});
    } else if (status == sim::DiplomacyStatus::Rival) {
        dialogueText = "\"Your banners stand too close to mine.\nSpeak quickly.\"";
        dialogueOptions.push_back({"Declare war.", [this, pKID, rKID]() { 
            sim::WarfareManager::declareWar(simulationManager->getRegistry(), pKID, rKID, "Diplomatic breakdown.");
            endDiplomaticDialogue(); 
        }});
        dialogueOptions.push_back({"Discuss terms.", [this]() { endDiplomaticDialogue(); }});
    } else if (status == sim::DiplomacyStatus::Friendly || status == sim::DiplomacyStatus::Alliance) {
        dialogueText = "\"Welcome, friend. Our fires are yours.\nWhat news do you bring?\"";
        dialogueOptions.push_back({"Discuss trade.", [this]() { endDiplomaticDialogue(); }});
        dialogueOptions.push_back({"Propose joint attack.", [this]() { endDiplomaticDialogue(); }});
    } else {
        dialogueText = "\"You have crossed into lands that are not yours.\nWhat business brings you here?\"";
        if (pKID != 0 && rKID != 0) {
            dialogueOptions.push_back({"Declare war.", [this, pKID, rKID]() { 
                sim::WarfareManager::declareWar(simulationManager->getRegistry(), pKID, rKID, "Declared war at a diplomatic meeting.");
                endDiplomaticDialogue(); 
            }});
        }
        dialogueOptions.push_back({"Discuss trade.", [this]() { endDiplomaticDialogue(); }});
    }

    // Insert the universal inspection option right before leaving
    dialogueOptions.push_back({"[ Inspect Character ]", [this]() { isInspectingCharacter = true; }});
    dialogueOptions.push_back({"Leave.", [this]() { endDiplomaticDialogue(); }});
}

void PlayState::drawCharacterProfile(sf::RenderWindow& window, sim::EntityID apeId) {
    sim::ApeData* ape = simulationManager->getRegistry().getApe(apeId);
    if (!ape) return;

    // --- 1. DATA EXTRACTION ---
    std::string name = ape->name;
    std::string ageHealth = "Age: " + std::to_string(static_cast<int>(ape->age)) + "   Health: " + std::to_string(static_cast<int>(ape->health));
    
    sim::DynastyData* dyn = simulationManager->getRegistry().getDynasty(ape->dynastyId);
    std::string dynastyName = dyn ? dyn->name + " Dynasty" : "Unlanded Wanderer";

    std::string title = "Villager";
    std::string realm = "Unknown Lands";
    
    sim::KingdomData* kData = nullptr;
    if (ape->currentKingdom != 0) {
        kData = simulationManager->getRegistry().getKingdom(ape->currentKingdom);
        if (kData) {
            realm = kData->name;
            title = (kData->currentKingId == ape->id) ? "King of " + realm : "Noble of " + realm;
        }
    } else if (ape->villageId != 0) {
        sim::VillageData* vData = simulationManager->getRegistry().getVillage(ape->villageId);
        if (vData) {
            realm = vData->name;
            title = (vData->leaderId == ape->id) ? "Chief of " + realm : "Villager of " + realm;
        }
    }

    // Process Traits
    std::string traitsStr = "Traits:\n";
    for (auto t : ape->traits) {
        switch(t) {
            case sim::Trait::Brave: traitsStr += "- Brave\n"; break;
            case sim::Trait::Coward: traitsStr += "- Coward\n"; break;
            case sim::Trait::Greedy: traitsStr += "- Greedy\n"; break;
            case sim::Trait::Honorable: traitsStr += "- Honorable\n"; break;
            case sim::Trait::Cruel: traitsStr += "- Cruel\n"; break;
            case sim::Trait::Charismatic: traitsStr += "- Charismatic\n"; break;
            case sim::Trait::Lazy: traitsStr += "- Lazy\n"; break;
            case sim::Trait::Strategic: traitsStr += "- Strategic\n"; break;
            case sim::Trait::Impulsive: traitsStr += "- Impulsive\n"; break;
            case sim::Trait::Curious: traitsStr += "- Curious\n"; break;
            case sim::Trait::Energetic: traitsStr += "- Energetic\n"; break;
        }
    }
    if (ape->traits.empty()) traitsStr += "- None\n";

    // --- 2. DYNAMIC OPINION CALCULATION (No backend bloat) ---
    int opinion = 0;
    std::string opinionReasons = "";
    sim::ApeData* player = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
    
    if (player && player->currentKingdom != 0 && kData != nullptr) {
        sim::KingdomData* pK = simulationManager->getRegistry().getKingdom(player->currentKingdom);
        if (pK) {
            // Base opinion off Diplomatic Status
            if (pK->relations.count(kData->id)) {
                sim::DiplomacyStatus status = pK->relations[kData->id];
                if (status == sim::DiplomacyStatus::War) { opinion -= 50; opinionReasons += "At War: -50\n"; }
                else if (status == sim::DiplomacyStatus::Rival) { opinion -= 20; opinionReasons += "Rivalry: -20\n"; }
                else if (status == sim::DiplomacyStatus::Alliance) { opinion += 40; opinionReasons += "Alliance: +40\n"; }
                else if (status == sim::DiplomacyStatus::Trade) { opinion += 15; opinionReasons += "Trade Partners: +15\n"; }
            }
            // Modify opinion based on active Border Tension
            if (pK->borderTension.count(kData->id)) {
                float tension = pK->borderTension[kData->id];
                if (tension > 0) {
                    int pen = static_cast<int>(tension / 5.f);
                    opinion -= pen;
                    opinionReasons += "Border Tension: -" + std::to_string(pen) + "\n";
                }
            }
        }
    }
    if (opinionReasons.empty()) opinionReasons = "Neutral: 0\n";
    std::string opinionStr = "Opinion of You: " + std::to_string(opinion) + "\n\nReasons:\n" + opinionReasons;

    float panelW = 320.f;
    float panelH = 500.f;
    float startX = profilePanelPos.x; 
    float startY = profilePanelPos.y;

    // Panel Background
    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(35, 25, 20, 245)); // Dark wood/parchment
    panel.setOutlineColor(sf::Color(180, 140, 70, 220)); // Dim gold outline
    panel.setOutlineThickness(2.f);
    window.draw(panel);

    // Lambda to easily draw aligned text inside the panel
    auto drawText = [&](const std::string& text, float x, float y, int size, sf::Color col, bool center) {
        sf::Text t(text, cinematicFont, size);
        t.setFillColor(col);
        t.setOutlineColor(sf::Color::Black);
        t.setOutlineThickness(1.5f);
        if (center) {
            sf::FloatRect bounds = t.getLocalBounds();
            t.setOrigin(bounds.left + bounds.width / 2.f, 0.f);
            t.setPosition(x + panelW / 2.f, y);
        } else {
            t.setPosition(x, y);
        }
        window.draw(t);
    };

    // Header Info
    float curY = startY + 25.f;
    drawText(name, startX, curY, 28, sf::Color(255, 215, 100), true); // Gold Name
    curY += 35.f;
    drawText(dynastyName, startX, curY, 16, sf::Color(180, 180, 180), true); // Silver Dynasty
    curY += 40.f;
    drawText(title, startX, curY, 20, sf::Color(220, 220, 220), true); // White Title
    curY += 35.f;
    drawText(ageHealth, startX, curY, 16, sf::Color(150, 200, 150), true); // Soft Green Stats
    curY += 40.f;

    // Decorative Divider
    sf::RectangleShape div(sf::Vector2f(panelW - 60.f, 2.f));
    div.setPosition(startX + 30.f, curY);
    div.setFillColor(sf::Color(120, 90, 50, 200));
    window.draw(div);
    curY += 20.f;

    // Traits
    drawText(traitsStr, startX + 30.f, curY, 16, sf::Color(200, 200, 200), false);
    curY += 130.f;

    // Decorative Divider
    div.setPosition(startX + 30.f, curY);
    window.draw(div);
    curY += 20.f;

    sf::Color opinionColor = (opinion < 0) ? sf::Color(255, 120, 120) : (opinion > 0 ? sf::Color(120, 255, 120) : sf::Color(200, 200, 200));
    drawText(opinionStr, startX + 30.f, curY, 16, opinionColor, false);

    curY = startY + panelH - 60.f;
    drawText("[ Back ]", startX, curY, 18, sf::Color(255, 255, 150), true);
}

void PlayState::drawWorldMap(sf::RenderWindow& window) {
    sf::View originalView = window.getView();
    window.setView(mapView);

    // Deep Parchment/Ocean Background
    sf::RectangleShape bg(sf::Vector2f(mapView.getSize().x, mapView.getSize().y));
    bg.setOrigin(bg.getSize().x / 2.f, bg.getSize().y / 2.f);
    bg.setPosition(mapView.getCenter());
    bg.setFillColor(sf::Color(210, 195, 160)); 
    window.draw(bg);

    // EXTRACT KNOWLEDGE
    std::unordered_set<sim::KingdomID> kKnown;
    std::unordered_set<sim::VillageID> vKnown;
    sim::ApeData* pData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
    
    if (pData) {
        if (pData->currentKingdom != 0) {
            kKnown.insert(pData->currentKingdom);
            sim::KingdomData* k = simulationManager->getRegistry().getKingdom(pData->currentKingdom);
            if (k) {
                for (auto id : k->knownKingdoms) kKnown.insert(id);
                for (auto id : k->controlledVillages) vKnown.insert(id);
            }
        } 
        if (pData->villageId != 0) {
            vKnown.insert(pData->villageId);
            sim::VillageData* v = simulationManager->getRegistry().getVillage(pData->villageId);
            if (v) for (auto id : v->knownVillages) vKnown.insert(id);
        }
        for (const auto& pair : simulationManager->getRegistry().getAllVillages()) {
            if (std::abs(pData->worldX - pair.second.centerX) < pair.second.territoryRadius + 800.f) {
                vKnown.insert(pair.first);
                if (pair.second.kingdomId != 0) kKnown.insert(pair.second.kingdomId);
            }
        }
    }

    // DRAW TERRAIN TOPOGRAPHY
    float viewLeft = mapView.getCenter().x - (mapView.getSize().x / 2.f);
    float viewRight = mapView.getCenter().x + (mapView.getSize().x / 2.f);
    sf::VertexArray ground(sf::TriangleStrip);
    for (float x = viewLeft - 2000.f; x <= viewRight + 2000.f; x += 500.f) {
        float y = worldManager->getTerrainHeight(x);
        ground.append(sf::Vertex(sf::Vector2f(x, y), sf::Color(160, 140, 100))); // Ground top
        ground.append(sf::Vertex(sf::Vector2f(x, y + 8000.f), sf::Color(110, 95, 70))); // Ground depth
    }
    window.draw(ground);

    // RENDER KINGDOM TERRITORIES
    for (auto kId : kKnown) {
        sim::KingdomData* k = simulationManager->getRegistry().getKingdom(kId);
        if (!k) continue;
        float width = k->territoryMaxX - k->territoryMinX;
        if (width > 0.f) {
            sf::RectangleShape rect(sf::Vector2f(width, 10000.f));
            rect.setOrigin(0.f, 5000.f);
            rect.setPosition(k->territoryMinX, worldManager->getTerrainHeight(k->territoryMinX));
            sf::Color c = k->color;
            c.a = 90; // Soft painted map territory
            rect.setFillColor(c);
            window.draw(rect);

            // Kingdom Banner Label
            sf::Text kLabel("Realm of " + k->name, cinematicFont, 240);
            kLabel.setFillColor(sf::Color(255, 255, 255, 220));
            kLabel.setOutlineColor(sf::Color::Black);
            kLabel.setOutlineThickness(10.f);
            sf::FloatRect kb = kLabel.getLocalBounds();
            kLabel.setOrigin(kb.left + kb.width / 2.f, kb.top + kb.height / 2.f);
            kLabel.setPosition(k->territoryMinX + width / 2.f, rect.getPosition().y - 1800.f);
            window.draw(kLabel);

            if (kId == selectedKingdomId) {
                sf::RectangleShape highlight(sf::Vector2f(width, 10000.f));
                highlight.setOrigin(0.f, 5000.f);
                highlight.setPosition(k->territoryMinX, rect.getPosition().y);
                highlight.setFillColor(sf::Color(255, 255, 255, 40)); // Bright overlay
                highlight.setOutlineColor(sf::Color(255, 215, 0)); // Gold border
                highlight.setOutlineThickness(30.f);
                window.draw(highlight);
            }

            for (auto aId : k->activeArmies) {
                sim::ArmyData* army = simulationManager->getRegistry().getArmy(aId);
                if (army) {
                    sf::ConvexShape flag(4);
                    flag.setPoint(0, sf::Vector2f(0.f, 0.f));
                    flag.setPoint(1, sf::Vector2f(200.f, 100.f));
                    flag.setPoint(2, sf::Vector2f(0.f, 200.f));
                    flag.setPoint(3, sf::Vector2f(0.f, 600.f));
                    flag.setFillColor(k->color);
                    flag.setOutlineColor(sf::Color::Black);
                    flag.setOutlineThickness(15.f);
                    flag.setPosition(army->worldX, worldManager->getTerrainHeight(army->worldX) - 600.f);
                    window.draw(flag);
                }
            }
        }
    }

    // RENDER VILLAGES & STRUCTURES
    for (auto vId : vKnown) {
        sim::VillageData* v = simulationManager->getRegistry().getVillage(vId);
        if (!v) continue;
        float terrainY = worldManager->getTerrainHeight(v->centerX);
        
        // Draw Structures (Detail Pass)
        for (auto sId : v->finishedStructures) {
            sim::StructureData* s = simulationManager->getRegistry().getStructure(sId);
            if (s) {
                sf::RectangleShape sRect(sf::Vector2f(120.f, 180.f));
                sRect.setOrigin(60.f, 180.f);
                sRect.setPosition(s->worldX, worldManager->getTerrainHeight(s->worldX));
                sRect.setFillColor(sf::Color(100, 80, 50));
                sRect.setOutlineColor(sf::Color(30, 20, 10));
                sRect.setOutlineThickness(10.f);
                window.draw(sRect);
            }
        }

        // Draw Village Marker
        // Draw Village Marker
        bool isCapital = (v->kingdomId != 0 && kKnown.count(v->kingdomId) && simulationManager->getRegistry().getKingdom(v->kingdomId)->capitalVillageId == v->id);
        
        float radius = isCapital ? 200.f : 120.f;
        
        // Selection Highlight for Village
        if (vId == selectedVillageId) {
            sf::CircleShape selMarker(radius + 40.f, isCapital ? 4 : 30);
            selMarker.setOrigin(selMarker.getRadius(), selMarker.getRadius());
            selMarker.setPosition(v->centerX, terrainY - radius);
            selMarker.setFillColor(sf::Color::Transparent);
            selMarker.setOutlineColor(sf::Color(255, 255, 0)); // Bright Yellow/Gold
            selMarker.setOutlineThickness(25.f);
            window.draw(selMarker);
        }

        sf::CircleShape marker(radius, isCapital ? 4 : 30);
        marker.setOrigin(marker.getRadius(), marker.getRadius());
        marker.setPosition(v->centerX, terrainY - marker.getRadius());
        marker.setFillColor(isCapital ? sf::Color(220, 180, 40) : sf::Color(140, 150, 160));
        marker.setOutlineColor(sf::Color(20, 20, 20));
        marker.setOutlineThickness(20.f);
        window.draw(marker);

        // Village Label
        sf::Text vLabel(v->name, cinematicFont, isCapital ? 180 : 130);
        vLabel.setFillColor(sf::Color(250, 240, 220));
        vLabel.setOutlineColor(sf::Color(20, 15, 10));
        vLabel.setOutlineThickness(12.f);
        sf::FloatRect bounds = vLabel.getLocalBounds();
        vLabel.setOrigin(bounds.left + bounds.width / 2.f, 0.f);
        vLabel.setPosition(v->centerX, terrainY + 250.f);
        window.draw(vLabel);
    }

    // DRAW PLAYER
    if (pData) {
        sf::CircleShape pMarker(150.f, 3);
        pMarker.setOrigin(150.f, 150.f);
        pMarker.setPosition(pData->worldX, pData->worldY);
        pMarker.setFillColor(sf::Color::White);
        pMarker.setOutlineColor(sf::Color::Black);
        pMarker.setOutlineThickness(25.f);
        window.draw(pMarker);
    }

    // DRAW UI BORDER & HINTS
    window.setView(window.getDefaultView());
    sf::Vector2f winSize(window.getSize().x, window.getSize().y);
    sf::FloatRect vp = mapView.getViewport();
    
    sf::RectangleShape border(sf::Vector2f(winSize.x * vp.width, winSize.y * vp.height));
    border.setPosition(winSize.x * vp.left, winSize.y * vp.top);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color(90, 60, 40));
    border.setOutlineThickness(8.f);
    window.draw(border);

    // If fully expanded or detached, show controls
    if (mapMode == MapMode::Expanded || isMapDetached) {
        sf::Text controls("[Mouse Drag] Pan   [Scroll] Zoom   [C] Recenter Player   [Tab] Resize Map", cinematicFont, 16);
        controls.setFillColor(sf::Color::White);
        controls.setOutlineColor(sf::Color::Black);
        controls.setOutlineThickness(2.f);
        controls.setPosition(border.getPosition().x + 10.f, border.getPosition().y + border.getSize().y - 25.f);
        window.draw(controls);
    }

    window.setView(originalView);
}

void PlayState::handleMapClick(sf::Vector2f worldPos) {
    // 1. EXTRACT STRICT FOG OF WAR KNOWLEDGE (Same as rendering)
    std::unordered_set<sim::KingdomID> kKnown;
    std::unordered_set<sim::VillageID> vKnown;
    sim::ApeData* pData = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
    
    if (pData) {
        if (pData->currentKingdom != 0) {
            kKnown.insert(pData->currentKingdom);
            sim::KingdomData* k = simulationManager->getRegistry().getKingdom(pData->currentKingdom);
            if (k) {
                for (auto id : k->knownKingdoms) kKnown.insert(id);
                for (auto id : k->controlledVillages) vKnown.insert(id);
            }
        } 
        if (pData->villageId != 0) {
            vKnown.insert(pData->villageId);
            sim::VillageData* v = simulationManager->getRegistry().getVillage(pData->villageId);
            if (v) for (auto id : v->knownVillages) vKnown.insert(id);
        }
        for (const auto& pair : simulationManager->getRegistry().getAllVillages()) {
            if (std::abs(pData->worldX - pair.second.centerX) < pair.second.territoryRadius + 800.f) {
                vKnown.insert(pair.first);
                if (pair.second.kingdomId != 0) kKnown.insert(pair.second.kingdomId);
            }
        }
    }

    // 2. CHECK SELECTION
    sim::VillageID hitVillage = 0;
    sim::KingdomID hitKingdom = 0;

    // Prioritize Villages (Smaller targets)
    for (auto vId : vKnown) {
        sim::VillageData* v = simulationManager->getRegistry().getVillage(vId);
        // 400.f is a generous click radius in world-space for the map markers
        if (v && std::abs(worldPos.x - v->centerX) < 400.f) {
            hitVillage = vId;
            break;
        }
    }

    // If no village hit, check Kingdom territories
    if (hitVillage == 0) {
        for (auto kId : kKnown) {
            sim::KingdomData* k = simulationManager->getRegistry().getKingdom(kId);
            if (k && worldPos.x >= k->territoryMinX && worldPos.x <= k->territoryMaxX) {
                hitKingdom = kId;
                break;
            }
        }
    }

    // Apply Selection (Clicking empty space clears selection)
    selectedVillageId = hitVillage;
    selectedKingdomId = hitKingdom;
}

void PlayState::drawVillageProfile(sf::RenderWindow& window, sim::VillageID vId) {
    sim::VillageData* v = simulationManager->getRegistry().getVillage(vId);
    if (!v) return;

    // --- DATA EXTRACTION ---
    std::string leaderName = "Unknown Leader";
    sim::ApeData* leader = simulationManager->getRegistry().getApe(v->leaderId);
    if (leader) leaderName = leader->name;

    std::string allegiance = "Independent";
    if (v->kingdomId != 0) {
        sim::KingdomData* k = simulationManager->getRegistry().getKingdom(v->kingdomId);
        if (k) allegiance = "Kingdom of " + k->name;
    }

    std::string popStr = std::to_string(v->members.size()) + " apes";
    std::string foodStr = std::to_string(v->food);
    std::string woodStr = std::to_string(v->wood);
    std::string stoneStr = std::to_string(v->stone);
    std::string activeProjects = std::to_string(v->constructionQueue.size());
    std::string statusStr = v->isMigrating ? "Migrating" : (v->food < v->members.size() ? "Hungry" : "Stable");

    float panelW = 340.f;
    float panelH = 500.f;
    float startX = profilePanelPos.x; 
    float startY = profilePanelPos.y;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(35, 25, 20, 245));
    panel.setOutlineColor(sf::Color(180, 140, 70, 220));
    panel.setOutlineThickness(2.f);
    window.draw(panel);

    auto drawText = [&](const std::string& text, float y, int size, sf::Color col, bool bold = false) {
        sf::Text t(text, cinematicFont, size);
        t.setFillColor(col);
        t.setOutlineColor(sf::Color::Black);
        t.setOutlineThickness(bold ? 2.f : 1.f);
        sf::FloatRect bounds = t.getLocalBounds();
        t.setOrigin(bounds.left + bounds.width / 2.f, 0.f);
        t.setPosition(startX + panelW / 2.f, y);
        window.draw(t);
    };

    float curY = startY + 25.f;
    drawText("VILLAGE OF " + v->name, curY, 26, sf::Color(255, 215, 100), true);
    
    curY += 40.f;
    sf::RectangleShape div(sf::Vector2f(panelW - 60.f, 2.f));
    div.setPosition(startX + 30.f, curY);
    div.setFillColor(sf::Color(120, 90, 50, 200));
    window.draw(div);

    curY += 20.f;
    drawText("Leader", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(leaderName, curY, 20, sf::Color::White);

    curY += 35.f;
    drawText("Allegiance", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(allegiance, curY, 18, sf::Color(200, 220, 255));

    curY += 35.f;
    drawText("Population", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(popStr, curY, 18, sf::Color::White);

    curY += 35.f;
    drawText("Stores ( Food / Wood / Stone )", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(foodStr + "  /  " + woodStr + "  /  " + stoneStr, curY, 18, sf::Color(150, 200, 150));

    curY += 35.f;
    drawText("Status: " + statusStr, curY, 18, sf::Color(220, 220, 220));

    // Button
    curY = startY + panelH - 60.f;
    drawText("[ View Leader ]", curY, 18, sf::Color(255, 255, 150), true);
}

void PlayState::drawKingdomProfile(sf::RenderWindow& window, sim::KingdomID kId) {
    sim::KingdomData* k = simulationManager->getRegistry().getKingdom(kId);
    if (!k) return;

    // --- DATA EXTRACTION ---
    std::string rulerName = "Unknown Ruler";
    sim::ApeData* ruler = simulationManager->getRegistry().getApe(k->currentKingId);
    if (ruler) rulerName = ruler->name;

    std::string dynastyName = "Unknown Dynasty";
    sim::DynastyData* dyn = simulationManager->getRegistry().getDynasty(k->leaderDynastyId);
    if (dyn) dynastyName = dyn->name;

    std::string capitalName = "Unknown Capital";
    sim::VillageData* cap = simulationManager->getRegistry().getVillage(k->capitalVillageId);
    if (cap) capitalName = cap->name;

    std::string popStr = std::to_string(k->population) + " apes";
    std::string villageStr = std::to_string(k->controlledVillages.size()) + " settlements";
    std::string milStr = std::to_string(k->militaryStrength) + " strength";
    std::string foodStr = std::to_string(k->treasuryFood);
    std::string woodStr = std::to_string(k->treasuryWood);
    std::string stoneStr = std::to_string(k->treasuryStone);

    // Relationship extraction
    std::string relStr = "Neutral";
    sf::Color relCol = sf::Color(200, 200, 200);
    sim::ApeData* pApe = simulationManager->getRegistry().getApe(simulationManager->getControlledApe());
    if (pApe && pApe->currentKingdom != 0 && pApe->currentKingdom != kId) {
        sim::KingdomData* pK = simulationManager->getRegistry().getKingdom(pApe->currentKingdom);
        if (pK && pK->relations.count(kId)) {
            switch (pK->relations[kId]) {
                case sim::DiplomacyStatus::War: relStr = "At War"; relCol = sf::Color(255, 100, 100); break;
                case sim::DiplomacyStatus::Rival: relStr = "Rival"; relCol = sf::Color(255, 150, 100); break;
                case sim::DiplomacyStatus::Alliance: relStr = "Alliance"; relCol = sf::Color(100, 200, 255); break;
                case sim::DiplomacyStatus::Trade: relStr = "Trade Partner"; relCol = sf::Color(150, 255, 150); break;
                case sim::DiplomacyStatus::Friendly: relStr = "Friendly"; relCol = sf::Color(180, 255, 180); break;
                default: break;
            }
        }
    } else if (pApe && pApe->currentKingdom == kId) {
        relStr = "Your Realm";
        relCol = sf::Color(255, 215, 100);
    }

    float panelW = 380.f;
    float panelH = 550.f;
    float startX = profilePanelPos.x; 
    float startY = profilePanelPos.y;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(startX, startY);
    panel.setFillColor(sf::Color(35, 25, 20, 245));
    panel.setOutlineColor(sf::Color(180, 140, 70, 220));
    panel.setOutlineThickness(2.f);
    window.draw(panel);

    auto drawText = [&](const std::string& text, float y, int size, sf::Color col, bool bold = false) {
        sf::Text t(text, cinematicFont, size);
        t.setFillColor(col);
        t.setOutlineColor(sf::Color::Black);
        t.setOutlineThickness(bold ? 2.f : 1.f);
        sf::FloatRect bounds = t.getLocalBounds();
        t.setOrigin(bounds.left + bounds.width / 2.f, 0.f);
        t.setPosition(startX + panelW / 2.f, y);
        window.draw(t);
    };

    float curY = startY + 25.f;
    drawText("KINGDOM OF " + k->name, curY, 28, k->color, true);
    
    curY += 40.f;
    sf::RectangleShape div(sf::Vector2f(panelW - 60.f, 2.f));
    div.setPosition(startX + 30.f, curY);
    div.setFillColor(sf::Color(120, 90, 50, 200));
    window.draw(div);

    curY += 20.f;
    drawText("Ruler", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(rulerName, curY, 22, sf::Color::White);
    curY += 24.f;
    drawText(dynastyName + " Dynasty", curY, 16, sf::Color(150, 150, 150));

    curY += 40.f;
    drawText("Capital", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(capitalName, curY, 18, sf::Color::White);

    curY += 35.f;
    drawText("Scale ( Pop / Villages )", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(popStr + "  |  " + villageStr, curY, 18, sf::Color::White);

    curY += 35.f;
    drawText("Treasury ( Food / Wood / Stone )", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(foodStr + "  /  " + woodStr + "  /  " + stoneStr, curY, 18, sf::Color(150, 200, 150));

    curY += 35.f;
    drawText("Military Power", curY, 14, sf::Color(180, 180, 180));
    curY += 18.f;
    drawText(milStr, curY, 18, sf::Color(255, 180, 180));

    curY += 40.f;
    drawText("Relationship: " + relStr, curY, 20, relCol, true);

    // Button
    curY = startY + panelH - 60.f;
    drawText("[ View Ruler ]", curY, 18, sf::Color(255, 255, 150), true);
}