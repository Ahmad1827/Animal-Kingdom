#pragma once

enum class ImpactLevel {
    None,
    Light,
    Medium,
    Heavy
};

struct LandingDetector {
    float squashTimer = 0.f;
    float squashDuration = 0.f;
    
    // Store the initial impact target scales
    float targetScaleX = 1.0f;
    float targetScaleY = 1.0f;
    
    // The current actual scale to render
    float squashScaleX = 1.0f;
    float squashScaleY = 1.0f;

    ImpactLevel registerLanding(float impactVelocityY);
    void updateSquash(float dt);
};