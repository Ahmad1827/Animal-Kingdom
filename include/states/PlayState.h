#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>

#include "states/GameState.h"
#include "world/Background.h"
#include "world/WorldManager.h"
#include "world/CameraManager.h"
#include "world/WeatherManager.h"
#include "world/DayNightCycle.h"
#include "world/WaterPlane.h"
#include "world/LightingManager.h"
#include "world/ParticleSystem.h"
#include "core/AudioManager.h"
#include "core/WorldClock.h"
#include "core/DebugOverlay.h"
#include "core/Profiler.h"
#include "simulation/SimulationManager.h"
#include "simulation/SimulationRegistry.h"
#include "world/StructureManager.h"
#include "world/NPCManager.h"
#include "interaction/InteractionManager.h"
#include "dynasty/DynastyUI.h"
#include "entities/Ape.h"

class Game;

enum class MapMode {
    Hidden,
    Mini,
    Expanded
};

struct DialogueOption {
    std::string text;
    std::function<void()> action;
};

struct CrowdProjectile {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float life = 2.0f;
    sf::Color color = sf::Color(255, 100, 150);
};

class PlayState : public GameState {
public:
    explicit PlayState(Game* game);
    ~PlayState() override = default;

    void init() override;
    void processEvents(const sf::Event& event) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;

private:
    Game* game;

    std::unique_ptr<Background> background;
    std::unique_ptr<WorldManager> worldManager;
    std::unique_ptr<CameraManager> cameraManager;
    std::unique_ptr<WeatherManager> weatherManager;
    std::unique_ptr<DayNightCycle> dayNightCycle;
    std::unique_ptr<LightingManager> lightingManager;
    std::unique_ptr<ParticleSystem> particleSystem;
    std::unique_ptr<AudioManager> audioManager;
    std::unique_ptr<WorldClock> worldClock;
    std::unique_ptr<DebugOverlay> debugOverlay;
    std::unique_ptr<sim::SimulationManager> simulationManager;
    std::unique_ptr<StructureManager> structureManager;
    std::unique_ptr<NPCManager> npcManager;
    std::unique_ptr<Ape> playerWrapper;

    InteractionManager interactionManager;
    sim::DynastyUI dynastyUI;
    ProfilerStats profiler;

    WaterPlane waterPlane;
    sf::RenderTexture sceneTexture;
    bool sceneTextureReady = false;

    uint32_t activeSeed = 0;
    sim::DynastyID activeDynastyId = 1;
    sim::VillageID activeClanId = 1;

    bool isTransitioning = false;
    float transitionTimer = 0.f;
    sf::Vector2f transitionTarget;

    sf::Font cinematicFont;
    std::string cinematicText;
    float cinematicTextTimer = 0.f;
    int currentPlayerKingdomId = -1;

    MapMode mapMode = MapMode::Hidden;
    sf::View mapView;
    sf::FloatRect currentViewport;
    sf::FloatRect targetViewport;
    float currentMapZoom = 6.0f;
    float targetMapZoom = 6.0f;
    sf::Vector2f mapCenter;
    bool isMapDetached = false;
    bool isDraggingMap = false;
    bool isDraggingProfile = false;
    sf::Vector2i lastMousePos;
    sf::Vector2i dragStartMousePos;
    sf::Vector2f profilePanelPos;
    bool isInspectingCharacter = false;
    sim::VillageID selectedVillageId = 0;
    sim::KingdomID selectedKingdomId = 0;

    bool isDialogueActive = false;
    sim::EntityID currentDialogueRepId = 0;
    std::string dialogueSpeakerName;
    std::string dialogueText;
    std::vector<DialogueOption> dialogueOptions;
    int dialogueSelectedIndex = 0;
    int currentDialogueNode = 0;
    float crowdSpawnTimer = 0.f;
    std::vector<CrowdProjectile> crowdProjectiles;

    float amberPulseTimer = 0.f;
    int lastObservedAmber = -1;

    bool f3PressedLastFrame = false;
    bool f4PressedLastFrame = false;
    bool f5PressedLastFrame = false;
    bool f6PressedLastFrame = false;
    bool f7PressedLastFrame = false;
    bool f8PressedLastFrame = false;
    bool f9PressedLastFrame = false;
    bool f10PressedLastFrame = false;
    bool f11PressedLastFrame = false;

    void initDynastySimulation();
    void refreshInteractionTargets();
    void startDiplomaticDialogue(sim::EntityID repId, int startNode = 0);
    void endDiplomaticDialogue();
    void loadDialogueNode(int nodeId);
    bool loadIntroNodes(int nodeId, sim::DiplomacyStatus status, float tension, sim::KingdomID pKID, sim::KingdomID rKID);
    bool loadDiscoveryNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID);
    bool loadNegotiationNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID);
    bool loadGrievanceNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID);
    bool loadEscalationNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID);
    bool loadVisitNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID);
    bool loadAudienceNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID);

    void drawWorldMap(sf::RenderWindow& window);
    void handleMapClick(sf::Vector2f worldPos);
    void drawCharacterProfile(sf::RenderWindow& window, sim::EntityID apeId);
    void drawVillageProfile(sf::RenderWindow& window, sim::VillageID vId);
    void drawKingdomProfile(sf::RenderWindow& window, sim::KingdomID kId);
    void drawAmberHUD(sf::RenderWindow& window, const sim::ApeData* playerApe);
};