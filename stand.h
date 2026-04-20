#include "queue/queue.h"
#include "queue.h"
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <iomanip>

using namespace std::chrono;

class Latch {
    std::atomic<int> count;
    std::atomic<bool> release_flag{false};
public:
    explicit Latch(int c) : count(c) {}
    void wait() {
        if (--count == 0) release_flag.store(true, std::memory_order_release);
        else while (!release_flag.load(std::memory_order_acquire)) std::this_thread::yield();
    }
};

struct TestMessage {
    int producer_id;
    int sequence_num;
};

struct ThreadMetrics {
    long long total_latency_ns = 0;
    long long max_latency_ns = 0;
    long long operations = 0;
};

void run_functional_test(int num_producers, int num_consumers, int ops_per_thread) {
    std::cout << "--- Starting functial testing (Producers: " << num_producers << " | Consumers: " << num_consumers << ") ---\n";
    LockFreeQueue<TestMessage> q;
    Latch start_latch(num_producers + num_consumers + 1);
    std::atomic<int> active_producers{num_producers};
    std::vector<std::vector<TestMessage>> consumer_results(num_consumers);

    auto producer = [&](int id) {
        start_latch.wait(); 
        for (int i = 0; i < ops_per_thread; ++i) q.enqueue({id, i});
        active_producers--;
    };

    auto consumer = [&](int c_id) {
        start_latch.wait();
        TestMessage msg;
        while (active_producers > 0 || q.dequeue(msg)) {
            if (q.dequeue(msg)) consumer_results[c_id].push_back(msg);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_producers; ++i) threads.emplace_back(producer, i);
    for (int i = 0; i < num_consumers; ++i) threads.emplace_back(consumer, i);

    start_latch.wait(); 
    for (auto& t : threads) t.join();

    std::vector<std::vector<int>> verification(num_producers);
    for (const auto& res : consumer_results) {
        for (const auto& msg : res) {
            verification[msg.producer_id].push_back(msg.sequence_num);
        }
    }

    bool passed = true;
    for (int i = 0; i < num_producers; ++i) {
        std::sort(verification[i].begin(), verification[i].end());
        if (verification[i].size() != static_cast<size_t>(ops_per_thread)) {
            std::cout << "Error: Loss or duplicate in thread #" << i << "!\n"; passed = false;
        }
        for (size_t j = 0; j < verification[i].size(); ++j) {
            if (verification[i][j] != static_cast<int>(j)) {
                std::cout << "Error: Order loss in thread #" << i << "!\n"; passed = false; break;
            }
        }
    }
    if (passed) std::cout << "Functional test succeded. Invariants were saved!\n\n";
}

template <typename QueueType>
void run_load_test(const std::string& name, int num_producers, int num_consumers, int ops_per_thread) {
    QueueType q;
    Latch start_latch(num_producers + num_consumers + 1);
    std::atomic<int> active_producers{num_producers};
    
    std::vector<ThreadMetrics> prod_metrics(num_producers);
    std::vector<ThreadMetrics> cons_metrics(num_consumers);

    auto producer = [&](int id) {
        start_latch.wait();
        for (int i = 0; i < ops_per_thread; ++i) {
            auto start = high_resolution_clock::now();
            q.enqueue(i);
            auto end = high_resolution_clock::now();
            long long lat = duration_cast<nanoseconds>(end - start).count();
            prod_metrics[id].total_latency_ns += lat;
            prod_metrics[id].max_latency_ns = std::max(prod_metrics[id].max_latency_ns, lat);
            prod_metrics[id].operations++;
        }
        active_producers--;
    };

    auto consumer = [&](int id) {
        start_latch.wait();
        int dummy;
        while (active_producers > 0 || q.dequeue(dummy)) {
            auto start = high_resolution_clock::now();
            if (q.dequeue(dummy)) {
                auto end = high_resolution_clock::now();
                long long lat = duration_cast<nanoseconds>(end - start).count();
                cons_metrics[id].total_latency_ns += lat;
                cons_metrics[id].max_latency_ns = std::max(cons_metrics[id].max_latency_ns, lat);
                cons_metrics[id].operations++;
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_producers; ++i) threads.emplace_back(producer, i);
    for (int i = 0; i < num_consumers; ++i) threads.emplace_back(consumer, i);

    auto test_start = high_resolution_clock::now();
    start_latch.wait();
    for (auto& t : threads) t.join();
    auto test_end = high_resolution_clock::now();

    double total_time_s = duration<double>(test_end - test_start).count();
    long long total_ops = num_producers * ops_per_thread;
    
    long long total_latency = 0;
    long long max_latency = 0;
    for (const auto& m : prod_metrics) { total_latency += m.total_latency_ns; max_latency = std::max(max_latency, m.max_latency_ns); }
    for (const auto& m : cons_metrics) { total_latency += m.total_latency_ns; max_latency = std::max(max_latency, m.max_latency_ns); }

    std::cout << "--- Results: " << name << " (" << num_producers << " Producers, " << num_consumers << " Consumers) ---\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total time:           " << total_time_s << " sec\n";
    std::cout << "Bandwidth: " << (total_ops / total_time_s) << " ops/sec\n";
    std::cout << "Avg latency:      " << (total_latency / (total_ops * 2.0)) << " ns/op\n"; 
    std::cout << "Max latency:        " << max_latency << " ns\n\n";
}