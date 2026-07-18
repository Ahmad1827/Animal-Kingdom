#include "world/CameraManager.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>

CameraManager::CameraManager(sf::Vector2f size) {
    view.setSize(size);
    view.setCenter(0.f, 0.f);
    lerpSpeed = 4.0f;
    shakeTrauma = 0.f;
    currentZoom = 1.35f; // Zoomed out by default for better platforming overview
    targetZoom = 1.35f;
}

void CameraManager::update(float dt, const sf::Vector2f& targetPos, const sf::Vector2f& targetVelocity) {
    // Dynamic lookahead scaling with speed
    float speedRatio = std::abs(targetVelocity.x) / 300.f;
    speedRatio = std::clamp(speedRatio, 0.f, 1.f);
    
    sf::Vector2f desiredLookAhead((targetVelocity.x > 0 ? 1 : -1) * 300.f * speedRatio, 0.f);
    lookAheadOffset.x += (desiredLookAhead.x - lookAheadOffset.x) * dt * 2.5f;
    
    sf::Vector2f idealPos = targetPos + lookAheadOffset;
    // Keep player in the lower-middle of the screen to plan horizontal and vertical jumps
    idealPos.y -= 50.f; 

    // Dynamic smoothing based on player action
    float currentLerpX = lerpSpeed + (speedRatio * 2.f);
    float currentLerpY = lerpSpeed;
    
    // Catch up quickly if falling fast, move slowly if standing still
    if (targetVelocity.y > 200.f) {
        currentLerpY = 8.0f;
    } else if (targetVelocity.x == 0.f && targetVelocity.y == 0.f) {
        currentLerpX = 2.0f;
        currentLerpY = 2.0f;
    }

    sf::Vector2f currentCenter = view.getCenter();
    sf::Vector2f newCenter;
    newCenter.x = currentCenter.x + (idealPos.x - currentCenter.x) * currentLerpX * dt;
    newCenter.y = currentCenter.y + (idealPos.y - currentCenter.y) * currentLerpY * dt;

    if (shakeTrauma > 0.f) {
        shakeOffset.x = ((std::rand() % 100) / 50.f - 1.f) * shakeTrauma * 20.f;
        shakeOffset.y = ((std::rand() % 100) / 50.f - 1.f) * shakeTrauma * 20.f;
        shakeTrauma -= dt;
        if (shakeTrauma < 0.f) shakeTrauma = 0.f;
    } else {
        shakeOffset = {0.f, 0.f};
    }

    currentZoom += (targetZoom - currentZoom) * dt * 3.0f;
    view.setSize(1280.f * currentZoom, 720.f * currentZoom);

    view.setCenter(newCenter + shakeOffset);
}

void CameraManager::addShake(float amount) { shakeTrauma += amount; }
void CameraManager::setZoom(float zoom) { targetZoom = zoom; }
const sf::View& CameraManager::getView() const { return view; }

sf::FloatRect CameraManager::getViewBounds() const {
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();
    return sf::FloatRect(center.x - size.x / 2.f, center.y - size.y / 2.f, size.x, size.y);
}