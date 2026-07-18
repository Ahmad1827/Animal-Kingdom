#include "world/CameraManager.h"
#include <cstdlib>

CameraManager::CameraManager(sf::Vector2f size) {
    view.setSize(size);
    view.setCenter(0.f, 0.f);
    lerpSpeed = 4.0f;
    shakeTrauma = 0.f;
}

void CameraManager::update(float dt, const sf::Vector2f& targetPos, const sf::Vector2f& targetVelocity) {
    sf::Vector2f desiredLookAhead(targetVelocity.x * 0.5f, 0.f);
    lookAheadOffset.x += (desiredLookAhead.x - lookAheadOffset.x) * dt * 2.f;
    
    sf::Vector2f idealPos = targetPos + lookAheadOffset;
    idealPos.y -= 150.f; // Look slightly above player

    sf::Vector2f currentCenter = view.getCenter();
    sf::Vector2f newCenter = currentCenter + (idealPos - currentCenter) * lerpSpeed * dt;

    if (shakeTrauma > 0.f) {
        shakeOffset.x = ((std::rand() % 100) / 50.f - 1.f) * shakeTrauma * 20.f;
        shakeOffset.y = ((std::rand() % 100) / 50.f - 1.f) * shakeTrauma * 20.f;
        shakeTrauma -= dt;
        if (shakeTrauma < 0.f) shakeTrauma = 0.f;
    } else {
        shakeOffset = {0.f, 0.f};
    }

    view.setCenter(newCenter + shakeOffset);
}

void CameraManager::addShake(float amount) {
    shakeTrauma += amount;
}

const sf::View& CameraManager::getView() const {
    return view;
}