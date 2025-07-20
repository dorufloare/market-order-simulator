#include "benchmark.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <limits>
#include <vector>
#include <cctype>

Benchmark &Benchmark::getInstance() {
    static Benchmark instance;
    return instance;
}

Benchmark::Benchmark() : programStart(std::chrono::high_resolution_clock::now()) {
    ensureLogsDirectory();

    if (loggingEnabled) {
        benchmarkFile.open("logs/benchmarks.log", std::ios::app);
        if (!benchmarkFile.is_open()) {
            std::cerr << "Warning: Could not open benchmarks.log file\n";
        } else {
            benchmarkFile << "\n=== NEW SESSION " << getCurrentTimestamp() << " ===\n";
            benchmarkFile << "Operation,AvgTime(ms),MinTime(ms),MaxTime(ms),Count,Throughput(ops/sec)\n";
        }
    }
}

Benchmark::~Benchmark() {
    if (benchmarkFile.is_open()) {
        benchmarkFile.close();
    }
}

void Benchmark::ensureLogsDirectory() {
    try {
        std::filesystem::create_directories("logs");
    } catch (const std::exception &e) {
        std::cerr << "Warning: Could not create logs directory: " << e.what() << "\n";
    }
}

std::string Benchmark::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

int Benchmark::getIndentLevel(const std::string &name) {
    static const std::vector<std::string> subOperations = {
        "Background", "Thread", "Pool", "Worker", "Internal", "Helper"
    };
    
    for (const auto &prefix : subOperations) {
        if (name.find(prefix) != std::string::npos) {
            return 1; 
        }
    }
    return 0; 
}

std::string Benchmark::formatDisplayName(const std::string &name) {
    int indent = getIndentLevel(name);
    std::string formatted = name;
    
    std::replace(formatted.begin(), formatted.end(), '_', ' ');
    std::string result = formatted + std::string(indent * 2, ' ');
    
    return result;
}

double Benchmark::getElapsedTimeMs(const std::chrono::high_resolution_clock::time_point &start) {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

Benchmark::Timer::Timer(const std::string &name)
    : timerName(name), startTime(std::chrono::high_resolution_clock::now()) {
}

Benchmark::Timer::~Timer() {
    auto endTime = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    Benchmark &benchmark = Benchmark::getInstance();
    std::lock_guard<std::mutex> lock(benchmark.benchmarkMutex);

    auto &timing = benchmark.timers[timerName];
    timing.totalTime += duration;
    timing.count++;
    timing.minTime = std::min(timing.minTime, duration);
    timing.maxTime = std::max(timing.maxTime, duration);
}

void Benchmark::startTimer(const std::string &name) {
    std::lock_guard<std::mutex> lock(benchmarkMutex);
    timers[name].startTime = std::chrono::high_resolution_clock::now();
}

void Benchmark::endTimer(const std::string &name) {
    auto endTime = std::chrono::high_resolution_clock::now();

    std::lock_guard<std::mutex> lock(benchmarkMutex);
    auto &timing = timers[name];

    if (timing.startTime.time_since_epoch().count() != 0) {
        double duration = std::chrono::duration<double, std::milli>(endTime - timing.startTime).count();
        timing.totalTime += duration;
        timing.count++;
        timing.minTime = std::min(timing.minTime, duration);
        timing.maxTime = std::max(timing.maxTime, duration);
        timing.startTime = std::chrono::high_resolution_clock::time_point{}; // Reset
    }
}

void Benchmark::incrementCounter(const std::string &name) {
    counters[name].fetch_add(1);
}

void Benchmark::addToCounter(const std::string &name, long value) {
    counters[name].fetch_add(value);
}

void Benchmark::recordThroughput(const std::string &operation, int count, double durationMs) {
    std::lock_guard<std::mutex> lock(benchmarkMutex);

    auto &throughput = throughputStats[operation];
    throughput.totalOperations += count;
    throughput.totalTime += durationMs;

    if (durationMs > 0) {
        double currentThroughput = (count * 1000.0) / durationMs; // ops per second
        throughput.peakThroughput = std::max(throughput.peakThroughput, currentThroughput);
    }

    throughput.lastUpdate = std::chrono::high_resolution_clock::now();
}

void Benchmark::displayRealTimeStats() {
    std::lock_guard<std::mutex> lock(benchmarkMutex);

    double totalProgramTime = getElapsedTimeMs(programStart);

    std::cout << "\n📊 === REAL-TIME PERFORMANCE STATS ===\n";
    std::cout << "Program Runtime: " << std::fixed << std::setprecision(2) << totalProgramTime / 1000.0 << "s\n\n";

    if (!counters.empty()) {
        std::cout << "📈 Counters:\n";
        for (const auto &[name, counter] : counters) {
            long value = counter.load();
            double rate = (totalProgramTime > 0) ? (value * 1000.0 / totalProgramTime) : 0;
            std::cout << "  " << formatDisplayName(name) << ": " << value << " (" << std::fixed << std::setprecision(1) << rate << "/sec)\n";
        }
        std::cout << "\n";
    }

    if (!timers.empty()) {
        std::cout << "⏱️  Timing Statistics:\n";
        std::cout << std::left << std::setw(35) << "Operation"
                  << std::setw(12) << "Count"
                  << std::setw(12) << "Avg(ms)"
                  << std::setw(12) << "Min(ms)"
                  << std::setw(12) << "Max(ms)"
                  << "Throughput(ops/sec)\n";
        std::cout << std::string(95, '-') << "\n";

        for (const auto &[name, timing] : timers) {
            if (timing.count > 0) {
                double avgTime = timing.totalTime / timing.count;
                double throughput = (timing.totalTime > 0) ? (timing.count * 1000.0 / timing.totalTime) : 0;

                std::cout << std::left << std::setw(35) << formatDisplayName(name)
                          << std::setw(12) << timing.count
                          << std::setw(12) << std::fixed << std::setprecision(3) << avgTime
                          << std::setw(12) << std::fixed << std::setprecision(3) << timing.minTime
                          << std::setw(12) << std::fixed << std::setprecision(3) << timing.maxTime
                          << std::fixed << std::setprecision(1) << throughput << "\n";
            }
        }
        std::cout << "\n";
    }

    if (!throughputStats.empty()) {
        std::cout << "🚀 Throughput Statistics:\n";
        for (const auto &[operation, data] : throughputStats) {
            double avgThroughput = (data.totalTime > 0) ? (data.totalOperations * 1000.0 / data.totalTime) : 0;
            std::cout << "  " << operation << ": "
                      << std::fixed << std::setprecision(1) << avgThroughput << " avg ops/sec"
                      << " (peak: " << data.peakThroughput << " ops/sec)\n";
        }
    }

    std::cout << "=======================================\n\n";
}

void Benchmark::displayFinalReport() {
    std::cout << "\n🏁 === FINAL PERFORMANCE REPORT ===\n";
    displayRealTimeStats();
    logBenchmarks();
}

void Benchmark::logBenchmarks() {
    if (!benchmarkFile.is_open())
        return;

    std::lock_guard<std::mutex> lock(benchmarkMutex);

    for (const auto &[name, timing] : timers) {
        if (timing.count > 0) {
            double avgTime = timing.totalTime / timing.count;
            double throughput = (timing.totalTime > 0) ? (timing.count * 1000.0 / timing.totalTime) : 0;

            benchmarkFile << name << ","
                          << std::fixed << std::setprecision(3) << avgTime << ","
                          << std::fixed << std::setprecision(3) << timing.minTime << ","
                          << std::fixed << std::setprecision(3) << timing.maxTime << ","
                          << timing.count << ","
                          << std::fixed << std::setprecision(1) << throughput << "\n";
        }
    }

    benchmarkFile.flush();
}

void Benchmark::reset() {
    std::lock_guard<std::mutex> lock(benchmarkMutex);
    timers.clear();
    counters.clear();
    throughputStats.clear();
    programStart = std::chrono::high_resolution_clock::now();
}

void Benchmark::enableLogging(bool enable) {
    loggingEnabled = enable;
}
