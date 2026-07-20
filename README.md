# Ape Dynasty

> **A procedurally generated 2D jungle survival & evolution game built with C++ and SFML.**

Ape Dynasty is an ambitious sandbox where you begin as a single ape in a living, infinite jungle. Explore procedurally generated environments, climb through dense canopies, survive against predators and rival tribes, establish your own dynasty, and eventually evolve from a lone survivor into the leader of an entire civilization.

---

# Current Development Status

**Version:** Prototype / Pre-Alpha

Current focus:
- Engine architecture
- Infinite procedural world
- Rendering pipeline
- Performance optimization
- Asset integration

Gameplay systems are currently being built on top of a completed world-generation framework.

---

# Features Implemented

## Infinite Procedural World

- Infinite chunk-based terrain generation
- Deterministic world generation using seeds
- Smooth procedural hills
- Region-based generation
- Automatic chunk loading and unloading
- Chunk caching
- Background chunk generation

---

## Rendering Engine

- Multi-layer rendering pipeline
- Separate gameplay and atmospheric rendering
- Frustum culling
- Layer ordering

Render Order:

Background

↓

Atmosphere

↓

Gameplay Geometry

↓

Player

↓

Particles

↓

Lighting

↓

Debug

---

## Camera System

Custom camera controller featuring:

- Dead-zone camera
- Smooth interpolation
- State-based camera speeds
- Stable vertical movement
- Improved slope handling
- Jitter fixes
- Cinematic movement

---

## World Generation

Current procedural generation includes:

- Jungle terrain
- Trees
- Branch platforms
- Decorative foliage
- Rocks
- Water bodies
- Region variation

Generation supports:

- Ancient forests
- Dense jungle
- Clearings
- Young forests

---

## Tree System

Procedural trees include:

- Variable trunk height
- Random branching
- Organic canopy generation
- Hanging vines
- Wind sway
- Sprite-ready architecture

---

## Environmental Systems

Implemented environmental effects include:

### Dynamic Weather

- Clear
- Rain
- Storm

Weather affects:

- Wind
- Tree sway
- Particle movement
- Ambient atmosphere

---

### Day / Night Cycle

Multi-stage lighting transition:

- Midnight
- Pre-Dawn
- Sunrise
- Morning
- Noon
- Afternoon
- Sunset
- Dusk
- Night

Lighting smoothly interpolates between states.

---

## Ambient Life

The jungle reacts dynamically through lightweight environmental simulation.

Includes:

- Fireflies
- Birds
- Butterflies
- Floating particles
- Leaf particles
- Wind-driven motion

---

## Interactive Environment

Player interactions include:

- Vine disturbance
- Tree sway
- Landing leaf particles
- Wind interaction

---

## Performance Optimizations

Implemented optimizations:

- Background chunk generation
- std::async chunk loading
- Chunk insertion queue
- Chunk cache
- Spatial partitioning
- O(1) collision queries
- View-frustum culling
- Reduced draw calls
- Camera optimization
- Slope jitter fixes

---

## Debug Tools

Runtime debugging suite:

F3

- World information

F4

- Chunk borders

F5

- Region visualization

F6

- Collision visualization

F7

- Toggle foliage

F8

- Performance profiler

F9

- Chunk streaming statistics

---

# Asset Pipeline

The engine now supports real production assets.

Features:

- Sprite-based rendering
- Bottom-center sprite anchoring
- Placeholder texture fallback
- Independent collision bounds
- Automatic scaling
- Artist-friendly asset replacement

This allows placeholder graphics to be replaced with final artwork without changing gameplay code.

---

# Player

Currently implemented:

- Walking
- Running
- Jumping
- Collision
- Camera following
- Terrain traversal

Currently integrating:

- Animated sprite sheets
- Climbing
- Vine swinging

---

# Planned Features

## Survival

- Hunger
- Thirst
- Stamina
- Injuries
- Diseases

---

## Wildlife

- Predators
- Herbivores
- Birds
- Insects

---

## Tribe System

- Family creation
- Social hierarchy
- Relationships
- Reproduction
- Generations

---

## Evolution

- Genetics
- Traits
- Mutations
- Intelligence
- Adaptation

---

## Building

- Sleeping nests
- Resource gathering
- Territory
- Defensive structures

---

## Combat

- Rival tribes
- Predators
- Territory control

---

## AI

Future plans include autonomous AI capable of:

- Exploration
- Resource gathering
- Social interaction
- Learning
- Reproduction
- Tribe expansion

---

# Technology

Language

- C++17

Framework

- SFML

Architecture

- Chunk-based world streaming
- Procedural generation
- Modular game systems
- Async loading
- Sprite rendering pipeline

---

# Project Goals

Ape Dynasty aims to combine ideas inspired by games such as:

- RimWorld
- Dwarf Fortress
- Rain World
- Terraria
- Noita

while creating a unique experience focused on:

- Evolution
- Procedural worlds
- Emergent storytelling
- Tribe simulation
- Infinite exploration

---

# Current Milestone

## Prototype Engine

Completed:

- Infinite world generation
- Chunk streaming
- Rendering pipeline
- Performance optimization
- Camera system
- Sprite-ready asset pipeline
- Dynamic atmosphere
- Environmental simulation

Current Work:

- Character sprite integration
- Animation system
- Gameplay mechanics

Next Milestone:

- Fully animated playable ape
- Climbing system
- Vine traversal
- Initial survival mechanics

---

# Screenshots

*(To be added)*

---

# License

This project is currently under active development.

All artwork and source code © Ape Dynasty.

---