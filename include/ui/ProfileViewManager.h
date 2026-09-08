#pragma once
#include <SFML/Graphics.hpp>
#include "simulation/SimulationRegistry.h"

class ProfileViewManager {
public:
    ProfileViewManager();

    void init(const sf::Font& font);
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& letterboxView, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);

    void draw(sf::RenderWindow& window, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);

    void inspectCharacter(sim::EntityID id);
    void inspectVillage(sim::VillageID id);
    void inspectKingdom(sim::KingdomID id);
    void close();

    bool isInspecting() const { return isInspectingCharacter || selectedVillageId != 0 || selectedKingdomId != 0; }
    sim::EntityID getInspectedApeId() const { return inspectedApeId; }

private:
    const sf::Font* font;
    bool isInspectingCharacter;
    sim::EntityID inspectedApeId;
    sim::VillageID selectedVillageId;
    sim::KingdomID selectedKingdomId;

    bool isDraggingProfile;
    sf::Vector2i lastMousePos;
    sf::Vector2f profilePanelPos;

    void drawCharacterProfile(sf::RenderWindow& window, sim::EntityID apeId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);
    void drawVillageProfile(sf::RenderWindow& window, sim::VillageID vId, sim::SimulationRegistry& reg);
    void drawKingdomProfile(sf::RenderWindow& window, sim::KingdomID kId, sim::SimulationRegistry& reg, sim::EntityID controlledApeId);
};