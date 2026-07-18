#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <memory>

class AudioManager {
private:
    std::unordered_map<std::string, std::unique_ptr<sf::Music>> musicTracks;
    std::unordered_map<std::string, sf::SoundBuffer> soundBuffers;
    float globalVolume;
    float currentWindIntensity;
    float currentRainIntensity;

public:
    AudioManager();
    void update(float dt, float wind, float rain, float timeOfDay);
    void playAmbient(const std::string& name);
    void stopAmbient(const std::string& name);
    void setGlobalVolume(float volume);
    // Architecture ready for SFX loading
};