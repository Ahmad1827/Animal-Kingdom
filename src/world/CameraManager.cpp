#include "world/CameraManager.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>

CameraManager::CameraManager(sf::Vector2f size) {
    view.setSize(size);
    view.setCenter(0.f, 0.f);
    shakeTrauma = 0.f;
    currentZoom = 1.35f;
    targetZoom = 1.35f;
    
    // Tuning parameters exposed for easy tweaking
    tuning.deadZone = {50.f, 150.f}; // Small horizontal, generous vertical deadzone
    tuning.lookAheadMax = 300.f;
    tuning.lookAheadSpeed = 2.0f;
    tuning.baseLerpX = 3.5f;
    tuning.baseLerpY = 4.0f;
    tuning.fallLerpY = 8.0f;
    tuning.climbLerp = 6.0f;
    tuning.airLerpX = 2.5f;
    tuning.basePreloadMargin = 2500.f;
    tuning.baseUnloadMargin = 5000.f;

    anchorPos = {0.f, 0.f};
}

void CameraManager::update(float dt, const sf::Vector2f& targetPos, const sf::Vector2f& targetVelocity, ApeState state) {
    // 1. Anchor System / Bounding Box (Dead Zone)
    if (targetPos.x > anchorPos.x + tuning.deadZone.x / 2.f) {
        anchorPos.x = targetPos.x - tuning.deadZone.x / 2.f;
    } else if (targetPos.x < anchorPos.x - tuning.deadZone.x / 2.f) {
        anchorPos.x = targetPos.x + tuning.deadZone.x / 2.f;
    }

    if (targetPos.y > anchorPos.y + tuning.deadZone.y / 2.f) {
        anchorPos.y = targetPos.y - tuning.deadZone.y / 2.f;
    } else if (targetPos.y < anchorPos.y - tuning.deadZone.y / 2.f) {
        anchorPos.y = targetPos.y + tuning.deadZone.y / 2.f;
    }

    // 2. Dynamic Look-ahead
    float speedRatio = std::abs(targetVelocity.x) / 300.f;
    speedRatio = std::clamp(speedRatio, 0.f, 1.f);
    
    float targetLookAheadX = 0.f;
    if (std::abs(targetVelocity.x) > 10.f) {
        targetLookAheadX = (targetVelocity.x > 0 ? 1.f : -1.f) * tuning.lookAheadMax * speedRatio;
    }
    lookAheadOffset.x += (targetLookAheadX - lookAheadOffset.x) * dt * tuning.lookAheadSpeed;

    // 3. Target Camera Position
    sf::Vector2f idealPos = anchorPos;
    idealPos.x += lookAheadOffset.x;
    idealPos.y -= 100.f; // Keep player in lower-middle

    // 4. State-Driven Interpolation Modes
    float currentLerpX = tuning.baseLerpX + (speedRatio * 1.5f);
    float currentLerpY = tuning.baseLerpY;

    if (state == ApeState::Airborne) {
        if (targetVelocity.y > 200.f) {
            currentLerpY = tuning.fallLerpY; // Track falling rapidly
        } else {
            currentLerpY = tuning.baseLerpY * 0.4f; // Stabilize jump arcs
        }
        currentLerpX = tuning.airLerpX;
    } else if (state == ApeState::ClimbingTrunk || state == ApeState::ClimbingVine) {
        currentLerpX = tuning.climbLerp;
        currentLerpY = tuning.climbLerp; // Snappy follow when climbing
    } else if (targetVelocity.x == 0.f && targetVelocity.y == 0.f) {
        currentLerpX *= 0.5f;
        currentLerpY *= 0.5f; // Slow stabilization when resting
    }

    // 5. Apply Movement
    sf::Vector2f currentCenter = view.getCenter();
    float tX = 1.0f - std::exp(-currentLerpX * dt);
    float tY = 1.0f - std::exp(-currentLerpY * dt);

    sf::Vector2f newCenter;
    newCenter.x = currentCenter.x + (idealPos.x - currentCenter.x) * tX;
    newCenter.y = currentCenter.y + (idealPos.y - currentCenter.y) * tY;

    // FIX 1: Cut off Zeno's paradox micro-adjustments
    if (std::abs(idealPos.x - newCenter.x) < 0.5f) newCenter.x = idealPos.x;
    if (std::abs(idealPos.y - newCenter.y) < 0.5f) newCenter.y = idealPos.y;

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
    
    sf::Vector2f finalCenter = newCenter + shakeOffset;
    
    // FIX 2: Floor the final camera coordinates to enforce strict pixel-perfect rendering
    finalCenter.x = std::floor(finalCenter.x);
    finalCenter.y = std::floor(finalCenter.y);
    
    view.setCenter(finalCenter.x, finalCenter.y);
}

void CameraManager::addShake(float amount) { shakeTrauma += amount; }
void CameraManager::setZoom(float zoom) { targetZoom = zoom; }
const sf::View& CameraManager::getView() const { return view; }
CameraTuning& CameraManager::getTuning() { return tuning; }

sf::FloatRect CameraManager::getViewBounds() const {
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();
    return sf::FloatRect(center.x - size.x / 2.f, center.y - size.y / 2.f, size.x, size.y);
}

sf::FloatRect CameraManager::getPreloadBounds(const sf::Vector2f& playerVelocity) const {
    sf::FloatRect bounds = getViewBounds();
    // Dynamically increase margin if moving fast
    float dynamicExpandX = std::abs(playerVelocity.x) * 2.5f;
    float margin = tuning.basePreloadMargin + dynamicExpandX;
    
    bounds.left -= margin;
    bounds.top -= margin;
    bounds.width += margin * 2.f;
    bounds.height += margin * 2.f;
    return bounds;
}

sf::FloatRect CameraManager::getUnloadBounds() const {
    sf::FloatRect bounds = getViewBounds();
    float margin = tuning.baseUnloadMargin;
    bounds.left -= margin;
    bounds.top -= margin;
    bounds.width += margin * 2.f;
    bounds.height += margin * 2.f;
    return bounds;
}

sf::Vector2f CameraManager::getIdealPosition() const {
    sf::Vector2f ideal = anchorPos;
    ideal.x += lookAheadOffset.x;
    ideal.y -= 100.f;
    return ideal;
}