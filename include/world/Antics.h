#pragma once
// ---------------------------------------------------------------------------
// Antics - small things the Alpha can do for the fun of it. No menus, no cost
// worth worrying about, nothing to do with war.
//
//   R  Chest-beat   The Alpha hops and drums; every clan ape in earshot answers.
//                   Once a day it also lifts their opinion of you a little.
//   Q  Toss a treat A banana from the stores arcs out; the nearest ape of the
//                   clan trots over and eats it, and likes you better for it.
// ---------------------------------------------------------------------------
#include <SFML/Graphics.hpp>
#include <vector>
#include "simulation/SimulationRegistry.h"

class WorldManager;
class Ape;

class Antics {
public:
    void init();
    // Returns true if the key was one of ours.
    bool handleKey(sf::Keyboard::Key key, sim::SimulationRegistry& reg, sim::EntityID playerId, WorldManager* world, Ape* playerApe);
    void update(float dt, sim::SimulationRegistry& reg, sim::EntityID playerId, WorldManager* world);
    void draw(sf::RenderTarget& target);

private:
    enum Sprite { SPR_HEART, SPR_NOTE, SPR_BANANA, SPR_COUNT };

    struct Emote {              // a little mark that rises over an ape's head and fades
        float x = 0.f, y = 0.f;
        int   sprite = SPR_HEART;
        float delay = 0.f;      // seconds before it appears
        float age = 0.f;
    };

    struct Treat {
        bool  active = false;
        bool  landed = false;
        float x = 0.f, y = 0.f, vx = 0.f, vy = 0.f;
        float age = 0.f;
        sim::EntityID eater = 0;
    };

    sf::Texture atlas;
    bool atlasReady = false;
    sf::IntRect rects[SPR_COUNT];

    std::vector<Emote> emotes;
    Treat treat;
    float roarCooldown = 0.f;
    int   lastCheeredDay = -1;

    void addEmote(float x, float y, int sprite, float delay);
    void blit(sf::RenderTarget& target, int sprite, float cx, float cy, float scale, sf::Uint8 alpha) const;
};
