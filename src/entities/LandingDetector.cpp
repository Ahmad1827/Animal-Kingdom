#include "entities/LandingDetector.h"
#include <algorithm>
#include <iostream>

ImpactLevel LandingDetector::registerLanding(float impactVelocityY) {
    // We lowered the velocity thresholds slightly so standard jumps trigger Medium,
    // and high falls trigger Heavy.
    if (impactVelocityY > 700.f) {
        squashTimer = 0.15f;
        squashDuration = 0.15f;
        targetScaleX = 1.5f; // Extremely wide
        targetScaleY = 0.5f; // Extremely flat
        return ImpactLevel::Heavy;
    } else if (impactVelocityY > 400.f) {
        squashTimer = 0.12f;
        squashDuration = 0.12f;
        targetScaleX = 1.3f;
        targetScaleY = 0.7f;
        return ImpactLevel::Medium;
    } else if (impactVelocityY > 100.f) {
        squashTimer = 0.08f;
        squashDuration = 0.08f;
        targetScaleX = 1.15f;
        targetScaleY = 0.85f;
        return ImpactLevel::Light;
    }
    
    return ImpactLevel::None;
}

void LandingDetector::updateSquash(float dt) {
    if (squashTimer > 0.f) {
        squashTimer -= dt;
        float t = std::max(squashTimer / squashDuration, 0.0f);
        
        // FIX: Interpolate between 1.0 and the TARGET, not its own current value!
        squashScaleX = 1.0f + (targetScaleX - 1.0f) * t;
        squashScaleY = 1.0f + (targetScaleY - 1.0f) * t;
    } else {
        squashScaleX = 1.0f;
        squashScaleY = 1.0f;
    }
}