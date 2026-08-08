#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include "states/GameState.h"
#include "world/WorldManager.h"
#include "world/CameraManager.h"
#include "world/LightingManager.h"
#include "world/WeatherManager.h"
#include "world/ParticleSystem.h"
#include "world/Background.h"
#include "core/WorldClock.h"
#include "core/AudioManager.h"
#include "entities/Ape.h"
#include "core/Profiler.h"
#include "core/DebugOverlay.h"
#include "simulation/SimulationManager.h"
#include "world/NPCManager.h"
#include "world/StructureManager.h"
#include "interaction/InteractionManager.h"
#include <functional>

class Game;

class PlayState : public GameState {
private:
    Game* game;
    std::unique_ptr<Background> background;
    std::unique_ptr<Ape> playerWrapper;
    std::unique_ptr<WorldManager> worldManager;
    std::unique_ptr<CameraManager> cameraManager;
    std::unique_ptr<LightingManager> lightingManager;
    std::unique_ptr<WeatherManager> weatherManager;
    std::unique_ptr<ParticleSystem> particleSystem;
    std::unique_ptr<AudioManager> audioManager;
    std::unique_ptr<WorldClock> worldClock;
    std::unique_ptr<DebugOverlay> debugOverlay;
    
    std::unique_ptr<sim::SimulationManager> simulationManager;
    std::unique_ptr<NPCManager> npcManager;
    std::unique_ptr<StructureManager> structureManager;
    
    InteractionManager interactionManager;

    ProfilerStats profiler;
    uint32_t activeSeed;

    bool isTransitioning;
    float transitionTimer;
    sf::Vector2f transitionTarget;
    size_t lastKingdomCount = 99999; // Set high to force a refresh on frame 1
    void refreshInteractionTargets();
    bool f3PressedLastFrame;
    bool f4PressedLastFrame;
    bool f5PressedLastFrame;
    bool f6PressedLastFrame;
    bool f7PressedLastFrame;
    bool f8PressedLastFrame;
    bool f9PressedLastFrame;
    bool f10PressedLastFrame;
    bool f11PressedLastFrame;

    int currentPlayerKingdomId = -1;
    float cinematicTextTimer = 0.f;
    std::string cinematicText = "";
    sf::Font cinematicFont;

    struct DialogueOption {
        std::string text;
        std::function<void()> action;
    };
    bool isDialogueActive = false;
    int dialogueSelectedIndex = 0;
    std::string dialogueSpeakerName = "";
    std::string dialogueText = "";
    std::vector<DialogueOption> dialogueOptions;

    void startDiplomaticDialogue(sim::EntityID repId);
    void endDiplomaticDialogue();

public:
    PlayState(Game* game);
    void init() override;
    void processEvents(const sf::Event& event) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
};