#pragma once

#include <chrono>
#include <string>
#include <atomic>
#include <mutex>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <limits>

class Benchmark {
public:
    static Benchmark& getInstance();
    
    class Timer {
    public:
        Timer(const std::string& name);
        ~Timer();
        
    private:
        std::string timerName;
        std::chrono::high_resolution_clock::time_point startTime;
    };
    
    void startTimer(const std::string& name);
    void endTimer(const std::string& name);
    
    void incrementCounter(const std::string& name);
    void addToCounter(const std::string& name, long value);
    
    void recordThroughput(const std::string& operation, int count, double durationMs);
    
    void displayRealTimeStats();
    void displayFinalReport();
    void logBenchmarks();
    
    void reset();
    void enableLogging(bool enable);
    
private:
    Benchmark();
    ~Benchmark();
    
    Benchmark(const Benchmark&) = delete;
    Benchmark& operator=(const Benchmark&) = delete;
    
    struct TimingData {
        std::chrono::high_resolution_clock::time_point startTime;
        double totalTime = 0.0;
        long count = 0;
        double minTime = std::numeric_limits<double>::max();
        double maxTime = 0.0;
    };
    
    struct ThroughputData {
        long totalOperations = 0;
        double totalTime = 0.0;
        double peakThroughput = 0.0;
        std::chrono::high_resolution_clock::time_point lastUpdate;
    };
    
    std::unordered_map<std::string, TimingData> timers;
    std::unordered_map<std::string, std::atomic<long>> counters;
    std::unordered_map<std::string, ThroughputData> throughputStats;
    
    std::mutex benchmarkMutex;
    std::ofstream benchmarkFile;
    bool loggingEnabled = true;
    std::chrono::high_resolution_clock::time_point programStart;
    
    void ensureLogsDirectory();
    std::string getCurrentTimestamp();
    std::string formatDisplayName(const std::string& name);
    int getIndentLevel(const std::string& name);
    double getElapsedTimeMs(const std::chrono::high_resolution_clock::time_point& start);
};

#define BENCHMARK_TIMER(name) Benchmark::Timer timer(name)
#define BENCHMARK_FUNCTION() Benchmark::Timer timer(__FUNCTION__)
