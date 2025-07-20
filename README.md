# 🚀 Market Order Simulator

A fast, multi-threaded order book simulator built in C++17. I built this to learn how financial markets really work and practice building systems where every millisecond counts. Features real-time performance monitoring, detailed logging, and concurrent order processing.

## ✨ Key Features

### 🔥 **Fast Architecture**
- Multi-threaded engine that detects your CPU cores automatically
- Order book split into sections for better performance
- Background trading that generates 500+ orders per second

### 📊 **Performance Monitoring**
- Real-time timing measurements down to microseconds
- Tracks order processing speed and throughput
- Saves performance data to CSV files

### 🏗️ **Logging System**
- Records every order and trade to CSV files with timestamps
- Thread-safe logging that works across all components
- Creates log directories automatically

### 🎯 **Market Simulation**
- Supports LIMIT and MARKET order types
- Orders match based on price and time priority
- Partial order fills and resting orders in the book
- Real-time price updates from completed trades

---

## 🛠️ How to Build & Run

### What You Need
- C++17 compiler (GCC 7+, Clang 5+, or Visual Studio 2017+)
- CMake 3.16 or newer
- A system that supports threads

### Build Steps

```bash
# Get the code
git clone https://github.com/yourusername/market-order-simulator.git
cd market-order-simulator

# Build it
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run it
./OrderBookSimulator
```

### What You'll See
```
🚀 Starting Market Order Simulator with Performance Benchmarking...
🖥️  Hardware threads detected: 8
⚙️  Using 8 worker threads
🔥 Starting high-volume background trading simulation...
📈 Background generator started

=== Market Order Simulator ===
Commands:
- <BUY/SELL> <LIMIT/MARKET> <price> <quantity> : Place an order
- price : Show current last traded price
- help : Show detailed help
- stats : Show performance statistics
- quit : Exit the simulator

> 
```

---

## 🎮 How to Use

### Basic Trading
```bash
# Buy 100 shares at $99.50 or better
> BUY LIMIT 99.50 100
✓ Order submitted: BUY LIMIT 100 @ $99.50

# Sell 50 shares at current market price
> SELL MARKET 0 50
✓ Order submitted: SELL MARKET 50 @ $0.00

# Check what the last trade price was
> price
💰 Last Traded Price: $98.75
```

### Performance Stats
```bash
# See how fast everything is running
> stats
📊 === REAL-TIME PERFORMANCE STATS ===
Program Runtime: 3.55s

📈 Counters:
  Volume Traded: 319375 (90014.6/sec)
  Orders Matched: 1116 (314.5/sec)
  Background Orders Generated  : 1358 (382.7/sec)
  Orders Resting: 721 (203.2/sec)
  Orders Processed: 1358 (382.7/sec)

⏱️  Timing Statistics:
Operation                          Count       Avg(ms)     Min(ms)     Max(ms)     Throughput(ops/sec)
-----------------------------------------------------------------------------------------------
Background Order Generation        1358        0.102       0.010       1.033       9794.8
OrderBook Match                    1358        0.100       0.010       1.030       9997.4

=======================================

> quit
Exiting...
🛑 Stopping background generator...

🏁 === FINAL PERFORMANCE REPORT ===
📊 === REAL-TIME PERFORMANCE STATS ===
Program Runtime: 6.15s

📈 Counters:
  Volume Traded: 587991 (95677.1/sec)
  Orders Matched: 2097 (341.2/sec)
  Background Orders Generated  : 2347 (381.9/sec)
  Orders Resting: 1179 (191.8/sec)
  Orders Processed: 2347 (381.9/sec)

⏱️  Timing Statistics:
Operation                          Count       Avg(ms)     Min(ms)     Max(ms)     Throughput(ops/sec)
-----------------------------------------------------------------------------------------------
Order Queue Wait                   1           6146.214    6146.214    6146.214    0.2
Background Order Generation        2347        0.105       0.010       3.241       9521.6
OrderBook Match                    2347        0.103       0.010       3.238       9712.6

=======================================
```

### Help
```bash
> help
=== Order Book Trading Help ===
Order Types:
• LIMIT orders: Execute only at your specified price or better
• MARKET orders: Execute immediately at best available price

Examples:
• BUY LIMIT 100.25 500   - Buy 500 shares at $100.25 or better
• SELL MARKET 0 200      - Sell 200 shares at current market price
• price                  - Show last traded price
• stats                  - Display performance metrics
• quit                   - Exit simulator
```

---

## 📈 Performance Results

### Actual Numbers
Real performance from testing:

```
📊 Performance Results:
┌────────────────────────────────────────────────────────┐
│ Metric                   │ Value                       │
├────────────────────────────────────────────────────────┤
│ Order Processing Rate    │ 380+ orders/sec             │
│ Average Latency          │ 0.10ms per order            │
│ Peak Throughput          │ 9,997+ ops/sec              │
│ Volume Processing        │ 95,677+ trades/sec          │
│ Order Matching Rate      │ 341+ matches/sec            │
│ Background Orders/sec    │ 382 (2ms sleep)             │
└────────────────────────────────────────────────────────┘
```

### Key Numbers
```
- OrderBook Match:         9,997 ops/sec  
- Background Generation:   9,794 ops/sec
- Volume Traded:          95,677 trades/sec
- Average latency:        0.10ms
```

---

## 🏗️ How It Works

The system runs multiple threads:
- Main thread handles user input
- Engine thread manages the order queue
- Worker threads process orders in parallel
- Background thread generates fake market activity

Orders flow like this:
```
User Input → Engine Queue → Worker Threads → Order Book
Background Generator → Order Book (direct)
All activity gets logged to CSV files
```

---

## 📊 Log Files

The program creates these log files in the `build/logs/` directory:

### orders.log
```csv
Timestamp,OrderID,UserID,Type,Side,Price,Quantity,Status
2025-07-20 12:37:15.708,10000,8438,LIMIT,SELL,119.21,6.13,SUBMITTED
2025-07-20 12:37:15.710,10000,8438,LIMIT,SELL,119.21,6.13,RESTING
2025-07-20 12:37:15.762,10001,2967,LIMIT,BUY,106.08,1.73,SUBMITTED
2025-07-20 12:37:15.763,10001,2967,LIMIT,BUY,106.08,1.73,RESTING
```

### matches.log
```csv
Timestamp,IncomingOrderID,RestingOrderID,MatchPrice,MatchQuantity,IncomingSide,RestingSide
2025-07-20 12:37:16.125,10002,10000,119.21,2.50,BUY,SELL
2025-07-20 12:37:16.847,10008,10003,106.08,1.73,SELL,BUY
```

### benchmarks.log
```csv
Operation,AvgTime(ms),MinTime(ms),MaxTime(ms),Count,Throughput(ops/sec)
Order_Queue_Wait,6146.214,6146.214,6146.214,1,0.2
Background_Order_Generation,0.105,0.010,3.241,2347,9521.6
OrderBook_Match,0.103,0.010,3.238,2347,9712.6
```

---

## 📄 License

This project is open source and available under the [MIT License](LICENSE).