#include "entities/NPCApe.h"
#include "entities/Tree.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <iostream>

NPCApe::NPCApe(sim::EntityID id, float x, float y, sf::Texture& texture) 
    : simId(id), physicalApe(x, y, texture, false), stateTimer(0.f), intendedMoveX(0.f), 
      isDroppingToHang(false), grabbedChunk(0), grabbedVine(-1), grabbedSeg(-1), 
      pauseTimer(0.f), workTimer(0.f), woodcutLogTimer(0.f), fontLoaded(false) {
    baseSpeedMultiplier = 1.0f;
    personalOffset = ((id * 37) % 100) - 50.f; 

    if (nameFont.loadFromFile("assets/fonts/PressStart2P-Regular.ttf") ||
        nameFont.loadFromFile("assets/fonts/font.ttf") ||
        nameFont.loadFromFile("assets/fonts/Cinzel-Regular.ttf") ||
        nameFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")) {
        fontLoaded = true;
        nameText.setFont(nameFont);
        nameText.setCharacterSize(11);
        nameText.setFillColor(sf::Color::Yellow);
        nameText.setOutlineColor(sf::Color::Black);
        nameText.setOutlineThickness(1.5f);
    }
}

bool NPCApe::hasTrait(sim::ApeData* data, sim::Trait trait) {
    for (auto t : data->traits) {
        if (t == trait) return true;
    }
    return false;
}

void NPCApe::fireAudioHook(const std::string& soundEvent) {
}

void NPCApe::determineNextAction(sim::ApeData* data, float timeOfDay, sim::SimulationRegistry& registry, sim::EntityID playerId) {
    if (data->currentJob == sim::Job::Woodcutter) {
        return;
    }

    sim::VillageData* v = registry.getVillage(data->villageId);

    if (data->carriedType == sim::ResourceType::Food) physicalApe.setCarriedItem(1);
    else if (data->carriedType == sim::ResourceType::Wood) physicalApe.setCarriedItem(2);
    else if (data->carriedType == sim::ResourceType::Stone) physicalApe.setCarriedItem(3);
    else physicalApe.setCarriedItem(0);

    if (std::rand() % 100 < 5 && pauseTimer <= 0.f) {
        pauseTimer = 0.5f + (std::rand() % 15) / 10.f;
        intendedMoveX = 0.f;
        physicalApe.setState(ApeState::Grounded);
        return;
    }

    float targetX = physicalApe.getPosition().x;
    float stopDistance = 15.f;

    if (data->hasTravelDestination) {
        targetX = data->travelDestinationX;
        stopDistance = 10.f;
    } else if (v) {
        targetX = v->centerX + personalOffset;
        stopDistance = 15.f;
    }

    if (std::abs(physicalApe.getPosition().x - targetX) > stopDistance) {
        intendedMoveX = (physicalApe.getPosition().x < targetX) ? 1.f : -1.f;
        physicalApe.setState(ApeState::Grounded);
    } else {
        intendedMoveX = 0.f;
        physicalApe.setState(ApeState::Grounded);
    }
}

void NPCApe::applyPhysics(float dt, WorldManager* worldManager) {
    sf::FloatRect playerBounds = physicalApe.getBounds();
    sf::FloatRect platformBounds;
    bool wasGrounded = (physicalApe.getState() == ApeState::Grounded);

    if (physicalApe.getState() != ApeState::ClimbingTrunk && physicalApe.getState() != ApeState::HangingBranch && physicalApe.getState() != ApeState::ClimbingVine) {
        physicalApe.setState(ApeState::Airborne);
    }

    float groundHeight = worldManager->getTerrainHeight(playerBounds.left + playerBounds.width / 2.f);
    float bottomY = playerBounds.top + playerBounds.height;
    float distanceToGround = groundHeight - bottomY;
    
    if (physicalApe.getVelocity().y >= 0.f && bottomY >= groundHeight) {
        physicalApe.setPosition(physicalApe.getPosition().x, groundHeight - playerBounds.height);
        physicalApe.setVelocity(physicalApe.getVelocity().x, 0.f);
        physicalApe.setState(ApeState::Grounded);
        physicalApe.setDroppingThrough(false);
        isDroppingToHang = false;
    }
    else if (wasGrounded && physicalApe.getVelocity().y >= 0.f && distanceToGround > 0.f && distanceToGround < 25.f) {
        physicalApe.setPosition(physicalApe.getPosition().x, groundHeight - playerBounds.height);
        physicalApe.setVelocity(physicalApe.getVelocity().x, 0.f);
        physicalApe.setState(ApeState::Grounded);
        physicalApe.setDroppingThrough(false);
        isDroppingToHang = false;
    }

    sf::FloatRect checkBounds = playerBounds;
    sf::Vector2f checkVel = physicalApe.getVelocity();
    if (wasGrounded) { checkBounds.top += 2.f; if (checkVel.y == 0.f) checkVel.y = 10.f; }

    if (physicalApe.getState() == ApeState::Airborne && !physicalApe.isDroppingThrough()) {
        if (worldManager->checkOneWayCollision(checkBounds, checkVel, dt, platformBounds)) {
            physicalApe.setPosition(physicalApe.getPosition().x, platformBounds.top - playerBounds.height);
            physicalApe.setVelocity(physicalApe.getVelocity().x, 0.f);
            physicalApe.setState(ApeState::Grounded);
            isDroppingToHang = false;
        }
    }
}

void NPCApe::update(float dt, sim::ApeData* data, WorldManager* worldManager, float timeOfDay, sim::SimulationRegistry& registry, sim::EntityID playerId) {
    if (fontLoaded && data) {
        nameText.setString(data->name);
        sf::FloatRect bounds = physicalApe.getBounds();
        sf::FloatRect textBounds = nameText.getLocalBounds();
        nameText.setOrigin(textBounds.left + textBounds.width / 2.f, textBounds.top + textBounds.height);
        nameText.setPosition(bounds.left + bounds.width / 2.f, bounds.top - 8.f);
    }

    if (data->currentJob == sim::Job::Woodcutter && worldManager) {
        float myX = physicalApe.getPosition().x;
        float destX = (data->travelDestinationX != 0.f) ? data->travelDestinationX : myX;
        float distToDest = std::abs(myX - destX);

        woodcutLogTimer += dt;
        if (woodcutLogTimer >= 0.5f) {
            woodcutLogTimer = 0.f;
            std::cout << "[WOODCUTTER " << data->id << " (" << data->name << ")] CurrentX=" 
                      << myX << " | TargetTreeX=" << destX << " | Dist=" << distToDest << std::endl << std::flush;
        }

        if (distToDest > 80.f) {
            physicalApe.setState(ApeState::Grounded);
            intendedMoveX = (myX < destX) ? 1.f : -1.f;
        } else {
            intendedMoveX = 0.f;
            physicalApe.setVelocity(0.f, 0.f);

            int targetTreeId = static_cast<int>(data->currentTargetNode);

            if (targetTreeId != 0) {
                worldManager->harvestTree(targetTreeId);
            }
            worldManager->harvestTreeNear(destX, 150.f);
            worldManager->harvestTreeNear(myX, 150.f);

            data->currentJob = sim::Job::Idle;
            data->currentTargetNode = 0;
            data->hasTravelDestination = false;
            physicalApe.setState(ApeState::Grounded);

            std::cout << "[WOODCUT SUCCESS] Worker " << data->id << " reached Tree at X=" << destX << ". Tree deleted! Worker returning to IDLE." << std::endl << std::flush;
            return;
        }
    } else {
        if (pauseTimer > 0.f) {
            pauseTimer -= dt;
            intendedMoveX = 0.f; 
        } else {
            stateTimer -= dt;
            if (stateTimer <= 0.f) {
                determineNextAction(data, timeOfDay, registry, playerId);
                stateTimer = 0.2f + (std::rand() % 10) / 100.f; 
            }
        }
    }

    if (physicalApe.getState() == ApeState::Grounded || physicalApe.getState() == ApeState::Airborne) {
        float speed = 140.f * baseSpeedMultiplier;
        if (intendedMoveX != 0.f) {
            physicalApe.setVelocity(intendedMoveX * speed, physicalApe.getVelocity().y);
        } else {
            physicalApe.setVelocity(0.f, physicalApe.getVelocity().y);
        }
    }

    applyPhysics(dt, worldManager);
    physicalApe.update(dt);

    data->worldX = physicalApe.getPosition().x;
    data->worldY = physicalApe.getPosition().y;
    data->currentChunkX = static_cast<int>(std::floor(data->worldX / 2000.f));
}

void NPCApe::draw(sf::RenderTarget& target) {
    physicalApe.draw(target);
    if (fontLoaded) {
        target.draw(nameText);
    }
}