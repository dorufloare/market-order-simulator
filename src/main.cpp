#include <iostream>
#include <thread>
#include <algorithm>

#include "background_generator.hpp"
#include "engine.hpp"
#include "ui.hpp"
#include "benchmark.hpp"

int main() {
    std::cout << "🚀 Starting Market Order Simulator with Performance Benchmarking...\n";
    
    size_t hardware_threads = std::thread::hardware_concurrency();
    size_t optimal_threads = std::max(static_cast<size_t>(4), hardware_threads);
    
    std::cout << "Hardware threads detected: " << hardware_threads << "\n";
    std::cout << "Using " << optimal_threads << " worker threads\n";
    
    Engine engine(optimal_threads);
    engine.start();
    
    std::cout << "Starting high-volume background trading simulation...\n";
    
    BackgroundGenerator bgGenerator(engine.getOrderBook());
    bgGenerator.start();
    std::cout << "Background generator started\n";

    UI ui(engine);
    ui.start();

    std::cout << "Stopping background generator...\n";
    bgGenerator.stop();
    
    engine.stop();
    
    Benchmark::getInstance().displayFinalReport();

    return 0;
}
