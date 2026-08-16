# Order Book Matching Engine

A limit order book matching engine in C++17, supporting market and limit
orders, price-time priority, partial fills, order cancellation, and
thread-safe concurrent order ingestion, with a benchmark harness measuring
throughput and latency under single-threaded, per-order concurrent, and
batched concurrent execution.

## Build & test

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
ctest -C Release --output-on-failure
build\Release\benchmark.exe          # (or ./benchmark on Linux/macOS)
```

CMake automatically fetches Catch2 via `FetchContent` on first configure.

## Architecture

The core of the engine is `OrderBook`, which holds two `std::map<double,
std::deque<Order>>` structures. One for resting buy orders and one for resting
sell orders which keyed by price. The buy-side map uses `std::greater<double>`
as its comparator so the best (highest) bid is always at `begin()` and the
sell-side map uses the default ascending order so the best (lowest) ask is
always at `begin()`. Within a price level, orders are held in a `std::deque`
in arrival order, giving strict price-time priority: the earliest order at
a given price is always matched first.

When a new order arrives (`addOrder`), it's dispatched to a shared private
template, `matchAndRest`, parameterized on which side is the "opposite"
book to match against and which is "own" side to rest on if unfilled. The
function walks the opposite side's price levels from best to worst,
consuming resting orders (front of deque first) until either the incoming
order is fully filled, the opposite side is exhausted, or — for limit
orders only — the next price level no longer crosses the order's limit
price. Market orders have no such price limit and never rest; any
unfilled remainder is discarded rather than added to the book. Each match
produces a `Trade` recorded at the *resting* order's price, since the
resting order set the price the incoming order chose to cross.

Cancellation (`cancelOrder`) is supported via an auxiliary
`std::unordered_map<uint64_t, std::pair<double, Order::Side>>`
(`orderLocations_`) that maps an order id to its price and side, avoiding
a full scan of the book to find an order to cancel. This map is kept in
sync at every point an order starts or stops resting (on rest, on full
match, on cancel).

All public `OrderBook` methods lock a single internal `std::mutex`,
making the class safe to call concurrently from multiple threads. For
concurrent order ingestion, a custom `ConcurrentQueue<T>` (mutex +
`std::condition_variable`) connects a producer thread (order generator)
to a consumer thread that calls `OrderBook::addOrder` for each order
received. The queue supports both single-item and batched
push/pop, the latter added specifically to amortize per-item
lock/wake overhead across many orders at once (see Benchmark results).

## Benchmark results

Measured on a Windows/MSVC build, 100,000 synthetic limit orders per run
(fixed RNG seed for order generation; wall-clock timing, so throughput
varies roughly 5-15% run-to-run due to OS scheduling noise).

| Mode                              | Throughput (orders/sec) | p50 latency | p99 latency |
|------------------------------------|--------------------------|-------------|-------------|
| Single-threaded                    | ~3.6M - 4.4M             | 0.20 us     | 0.50-0.70 us|
| Producer/Consumer (per-order)      | ~2.9M - 3.0M             | 0.20 us     | 0.60-0.70 us|
| Producer/Consumer (batch=1024)     | ~3.3M - 3.6M                    | 0.20 us     | 0.60-0.70 us     |

## Additional notes


**Note on single-threaded execution outperforming concurrent
ingestion**: matching a single order in this engine takes on the order of
0.2 microseconds. The per-order concurrent path pays a full mutex
lock/unlock and `condition_variable` wake on *every single order* to move
it from producer to consumer, before any matching work even begins hence that
synchronization cost is comparable to or larger than the matching work
itself, so threading adds overhead rather than parallelism benefit at
this workload size. I.E threading
only pays off when per-item work is large relative to coordination cost.

**Note on Batching**: Batching amortizes the lock/wake cost
across many orders (e.g. 1024) instead of paying it per order, which is
why batched throughput approaches single-threaded throughput in these
results.

**Note on Raw throughput**:  In a system with heavier per-order work like risk checks, network
I/O per order, the same architecture would show a clearer parallelism
benefit.