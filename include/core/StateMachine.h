#pragma once
#include <memory>
#include <stack>
#include "states/GameState.h"

class StateMachine {
private:
    std::stack<std::unique_ptr<GameState>> states;
    std::unique_ptr<GameState> newState;
    bool isAdding;
    bool isRemoving;
    bool isReplacing;

public:
    StateMachine();
    void addState(std::unique_ptr<GameState> state, bool replace = true);
    void removeState();
    void processStateChanges();
    void processEvents(const sf::Event& event);
    void update(float dt);
    void draw(sf::RenderWindow& window);
};