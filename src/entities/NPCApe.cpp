#include "entities/NPCApe.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

NPCApe::NPCApe(sim::EntityID id, float x, float y, sf::Texture& texture) 
    : simId(id), physicalApe(x, y, texture, false), currentState(AIState::Idle), stateTimer(0.f), intendedMoveX(0.f), 
      isDroppingToHang(false), grabbedChunk(0), grabbedVine(-1), grabbedSeg(-1) {}

bool NPCApe::hasTrait(sim::ApeData* data, sim::Trait trait) {
    for (auto t : data->traits) {
        if (t == trait) return true;
    }
    return false;
}

void NPCApe::determineNextAction(sim::ApeData* data, float timeOfDay) {
    bool isNight = (timeOfDay > 0.8f || timeOfDay < 0.2f);

    if (isNight) {
        currentState = AIState::ReturningHome;
        float homeX = data->homeChunkX * 2000.f + 1000.f;
        intendedMoveX = (physicalApe.getPosition().x < homeX) ? 1.f : -1.f;
        stateTimer = 2.0f;
        
        if (std::abs(physicalApe.getPosition().x - homeX) < 200.f) {
            currentState = AIState::Idle;
            intendedMoveX = 0.f;
            stateTimer = 5.0f;
        }
        return;
    }

    int randChoice = std::rand() % 100;
    if (hasTrait(data, sim::Trait::Lazy)) randChoice -= 20; 
    if (hasTrait(data, sim::Trait::Energetic)) randChoice += 20;

    if (randChoice < 30) {
        currentState = AIState::Idle;
        intendedMoveX = 0.f;
        stateTimer = hasTrait(data, sim::Trait::Lazy) ? (std::rand() % 4 + 4.f) : (std::rand() % 3 + 1.f);
    } else {
        currentState = AIState::Wandering;
        intendedMoveX = (std::rand() % 2 == 0) ? 1.f : -1.f;
        stateTimer = hasTrait(data, sim::Trait::Brave) ? (std::rand() % 5 + 3.f) : (std::rand() % 3 + 1.f);
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
    
    if (physicalApe.getVelocity().y >= 0.f && bottomY >= groundHeight) {
        physicalApe.setPosition(physicalApe.getPosition().x, groundHeight - playerBounds.height);
        physicalApe.setVelocity(physicalApe.getVelocity().x, 0.f);
        physicalApe.setState(ApeState::Grounded);
        physicalApe.setDroppingThrough(false);
        isDroppingToHang = false;
    }

    sf::FloatRect checkBounds = playerBounds;
    sf::Vector2f checkVel = physicalApe.getVelocity();
    if (wasGrounded) {
        checkBounds.top += 2.f; 
        if (checkVel.y == 0.f) checkVel.y = 10.f; 
    }

    if (physicalApe.getState() == ApeState::Airborne && !physicalApe.isDroppingThrough()) {
        if (worldManager->checkOneWayCollision(checkBounds, checkVel, dt, platformBounds)) {
            physicalApe.setPosition(physicalApe.getPosition().x, platformBounds.top - playerBounds.height);
            physicalApe.setVelocity(physicalApe.getVelocity().x, 0.f);
            physicalApe.setState(ApeState::Grounded);
            isDroppingToHang = false;
        }
    }

    float trunkCenter = 0.f, trunkTop = 0.f;
    if (worldManager->checkTrunkCollision(playerBounds, trunkCenter, trunkTop)) {
        if (currentState == AIState::Climbing && physicalApe.getState() != ApeState::ClimbingTrunk) {
            if (physicalApe.getPosition().y >= trunkTop - 10.f) {
                physicalApe.setState(ApeState::ClimbingTrunk);
                physicalApe.setPosition(trunkCenter - (playerBounds.width / 2.f), physicalApe.getPosition().y);
                physicalApe.setVelocity(0.f, 0.f);
            }
        }
    } else if (physicalApe.getState() == ApeState::ClimbingTrunk) {
        physicalApe.setState(ApeState::Airborne);
    }
}

void NPCApe::update(float dt, sim::ApeData* data, WorldManager* worldManager, float timeOfDay) {
    stateTimer -= dt;
    if (stateTimer <= 0.f) {
        determineNextAction(data, timeOfDay);
    }

    if (physicalApe.getState() == ApeState::Grounded || physicalApe.getState() == ApeState::Airborne) {
        float speed = (currentState == AIState::Wandering || currentState == AIState::ReturningHome) ? 180.f : 0.f;
        if (intendedMoveX != 0.f) {
            physicalApe.setVelocity(intendedMoveX * speed, physicalApe.getVelocity().y);
        } else {
            physicalApe.setVelocity(0.f, physicalApe.getVelocity().y);
        }
    }

    applyPhysics(dt, worldManager);
    physicalApe.update(dt);

    // Sync Simulation Data
    data->worldX = physicalApe.getPosition().x;
    data->worldY = physicalApe.getPosition().y;
    data->currentChunkX = static_cast<int>(std::floor(data->worldX / 2000.f));
}

void NPCApe::draw(sf::RenderTarget& target) {
    physicalApe.draw(target);
}