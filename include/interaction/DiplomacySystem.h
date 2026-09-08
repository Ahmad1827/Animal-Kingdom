#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>
#include "simulation/SimulationRegistry.h"
#include "world/WorldManager.h"
#include "world/CameraManager.h"

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

class DiplomacySystem {
public:
    DiplomacySystem();

    void init(const sf::Font& font);
    void startDialogue(sim::EntityID repId, sim::SimulationRegistry& reg, sim::EntityID playerApeId, int startNode = 0);
    void endDialogue(sim::SimulationRegistry& reg, sim::EntityID playerApeId);

    void update(float dt, sim::SimulationRegistry& reg, sim::EntityID playerApeId, WorldManager* worldManager, CameraManager* cameraManager);
    bool handleEvent(const sf::Event& event);
    void draw(sf::RenderWindow& window);

    bool isActive() const { return isDialogueActive; }
    int getCurrentNode() const { return currentDialogueNode; }
    sim::EntityID getRepresentativeId() const { return currentDialogueRepId; }

private:
    const sf::Font* font;
    bool isDialogueActive;
    sim::EntityID currentDialogueRepId;
    std::string dialogueSpeakerName;
    std::string dialogueText;
    std::vector<DialogueOption> dialogueOptions;
    int dialogueSelectedIndex;
    int currentDialogueNode;

    float crowdSpawnTimer;
    std::vector<CrowdProjectile> crowdProjectiles;

    void loadDialogueNode(int nodeId, sim::SimulationRegistry& reg, sim::EntityID playerApeId);
    bool loadIntroNodes(int nodeId, sim::DiplomacyStatus status, float tension, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId);
    bool loadDiscoveryNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId);
    bool loadNegotiationNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId);
    bool loadGrievanceNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId);
    bool loadEscalationNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId);
    bool loadVisitNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId);
    bool loadAudienceNodes(int nodeId, sim::KingdomID pKID, sim::KingdomID rKID, sim::SimulationRegistry& reg, sim::EntityID playerApeId);
};