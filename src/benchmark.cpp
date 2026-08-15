#include "OrderBook.hpp"
#include "ConcurrentQueue.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <random>

using Clock = std::chrono::steady_clock;

// Generates N synthetic orders around a fixed midpoint price.
std::vector<Order> generateOrders(int n, uint64_t startId) {
    std::vector<Order> orders;
    orders.reserve(n);

    std::mt19937 rng(42); // fixed the seed so that the orders are reproducible
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> priceOffsetDist(-10, 10); // ticks around 100
    std::uniform_int_distribution<int> qtyDist(1, 20);

    for (int i = 0; i < n; ++i) {
        auto side = sideDist(rng) == 0 ? Order::Side::Buy : Order::Side::Sell;
        double price = 100.0 + priceOffsetDist(rng) * 0.5; // 0.5 tick size
        int qty = qtyDist(rng);
        orders.emplace_back(startId + i, side, Order::Type::Limit, price, qty);
    }
    return orders;
}

// Runs N orders through the book on the calling thread only. Returns
// (total elapsed seconds, per-order latency in microseconds).
std::pair<double, std::vector<double>> runSingleThreaded(OrderBook& book, std::vector<Order>& orders) {
    std::vector<double> latenciesUs;
    latenciesUs.reserve(orders.size());

    auto start = Clock::now();
    for (auto& order : orders) {
        auto t0 = Clock::now();
        book.addOrder(order);
        auto t1 = Clock::now();
        latenciesUs.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
    }
    auto end = Clock::now();

    double elapsedSec = std::chrono::duration<double>(end - start).count();
    return {elapsedSec, latenciesUs};
}

// Runs N orders through the book via a producer thread (pushes into the
// queue) and a consumer thread (pops and calls addOrder). Returns the
// same (elapsed, latencies) shape as the single-threaded version.
std::pair<double, std::vector<double>> runConcurrent(OrderBook& book, std::vector<Order>& orders) {
    ConcurrentQueue<Order> queue;
    std::vector<double> latenciesUs;
    latenciesUs.reserve(orders.size());

    auto start = Clock::now();

    std::thread producer([&]() {
        for (auto& order : orders) {
            queue.push(order);
        }
    });

    std::thread consumer([&]() {
        for (size_t i = 0; i < orders.size(); ++i) {
            auto t0 = Clock::now();
            Order o = queue.pop();
            book.addOrder(o);
            auto t1 = Clock::now();
            latenciesUs.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
        }
    });

    producer.join();
    consumer.join();

    auto end = Clock::now();
    double elapsedSec = std::chrono::duration<double>(end - start).count();
    return {elapsedSec, latenciesUs};
}

// Same idea as runConcurrent, but the producer/consumer transfer orders
// in batches instead of one at a time - amortizes lock/notify overhead
// across each batch instead of paying it per order.
std::pair<double, std::vector<double>> runConcurrentBatched(OrderBook& book, std::vector<Order>& orders, size_t batchSize) {
    ConcurrentQueue<Order> queue;
    std::vector<double> latenciesUs;
    latenciesUs.reserve(orders.size());

    auto start = Clock::now();

    std::thread producer([&]() {
        std::vector<Order> batch;
        batch.reserve(batchSize);
        for (auto& order : orders) {
            batch.push_back(order);
            if (batch.size() == batchSize) {
                queue.pushBatch(std::move(batch));
                batch.clear();
                batch.reserve(batchSize);
            }
        }
        if (!batch.empty()) {
            queue.pushBatch(std::move(batch));
        }
    });

    std::thread consumer([&]() {
        size_t received = 0;
        while (received < orders.size()) {
            auto batch = queue.popBatch(batchSize);
            for (auto& o : batch) {
                auto t0 = Clock::now();
                book.addOrder(o);
                auto t1 = Clock::now();
                latenciesUs.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
            }
            received += batch.size();
        }
    });

    producer.join();
    consumer.join();

    auto end = Clock::now();
    double elapsedSec = std::chrono::duration<double>(end - start).count();
    return {elapsedSec, latenciesUs};
}

// Computes and prints throughput + p50/p95/p99 latency for one run.
void printStats(const std::string& label, int numOrders, double elapsedSec, std::vector<double> latenciesUs) {
    std::sort(latenciesUs.begin(), latenciesUs.end());
    auto pct = [&](double p) {
        size_t idx = static_cast<size_t>(p * (latenciesUs.size() - 1));
        return latenciesUs[idx];
    };

    double throughput = numOrders / elapsedSec;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n=== " << label << " ===\n";
    std::cout << "  Orders:      " << numOrders << "\n";
    std::cout << "  Elapsed:     " << elapsedSec << " s\n";
    std::cout << "  Throughput:  " << throughput << " orders/sec\n";
    std::cout << "  Latency p50: " << pct(0.50) << " us\n";
    std::cout << "  Latency p95: " << pct(0.95) << " us\n";
    std::cout << "  Latency p99: " << pct(0.99) << " us\n";
}

int main() {
    const int N = 100000; // orders per run - adjust if it's too slow/fast on your machine

    // --- Single-threaded baseline ---
    {
        OrderBook book;
        auto orders = generateOrders(N, /*startId=*/1);
        auto [elapsed, latencies] = runSingleThreaded(book, orders);
        printStats("Single-threaded", N, elapsed, latencies);
    }

    // --- Producer/consumer concurrent version (one order at a time) ---
    {
        OrderBook book;
        auto orders = generateOrders(N, /*startId=*/1);
        auto [elapsed, latencies] = runConcurrent(book, orders);
        printStats("Producer/Consumer (per-order)", N, elapsed, latencies);
    }

    // --- Producer/consumer, batched transfer with batch size 256 ---
    {
        OrderBook book;
        auto orders = generateOrders(N, /*startId=*/1);
        const size_t batchSize = 256;
        auto [elapsed, latencies] = runConcurrentBatched(book, orders, batchSize);
        printStats("Producer/Consumer (batch=256)", N, elapsed, latencies);
    }

    // --- Producer/consumer, batched transfer with batch size 256 ---
    {
        OrderBook book;
        auto orders = generateOrders(N, /*startId=*/1);
        const size_t batchSize = 512;
        auto [elapsed, latencies] = runConcurrentBatched(book, orders, batchSize);
        printStats("Producer/Consumer (batch=512)", N, elapsed, latencies);
    }

    // --- Producer/consumer, batched transfer with batch size 1024 ---
    {
        OrderBook book;
        auto orders = generateOrders(N, /*startId=*/1);
        const size_t batchSize = 1024;
        auto [elapsed, latencies] = runConcurrentBatched(book, orders, batchSize);
        printStats("Producer/Consumer (batch=1024)", N, elapsed, latencies);
    }

    return 0;
}
