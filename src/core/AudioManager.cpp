#include "core/AudioManager.h"

AudioManager::AudioManager() : globalVolume(100.f), currentWindIntensity(0.f), currentRainIntensity(0.f) {
    // Architecture prepared. When assets are added, they map here.
}

void AudioManager::update(float dt, float wind, float rain, float timeOfDay) {
    currentWindIntensity = wind;
    currentRainIntensity = rain;
    // Future: Crossfade ambient forest tracks based on timeOfDay (birds at dawn, crickets at night)
    // Future: Modulate wind and rain volume based on intensity parameters
}

void AudioManager::playAmbient(const std::string& name) {
    auto it = musicTracks.find(name);
    if (it != musicTracks.end() && it->second->getStatus() != sf::Music::Playing) {
        it->second->play();
    }
}

void AudioManager::stopAmbient(const std::string& name) {
    auto it = musicTracks.find(name);
    if (it != musicTracks.end()) {
        it->second->stop();
    }
}

void AudioManager::setGlobalVolume(float volume) {
    globalVolume = volume;
}