#pragma once

#include "engine.hpp"
#include <atomic>
#include <thread>

class UI {
public:
    explicit UI(Engine& engine);
    ~UI();

    void start();
    void stop();

private:
    Engine& engine;
    std::atomic<bool> running;
    int orderId;

    void interactiveInput();
};
