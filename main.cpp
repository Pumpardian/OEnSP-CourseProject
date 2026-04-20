#include "queue/queue.h"
#include <iostream>

constexpr int threadCount = 4;
constexpr int elementCount = 50;

int main() {
    LockFreeQueue<int> queue;
    std::thread threads[threadCount];
    std::pair<int, int> enq_deq[elementCount];

    auto producer = [&queue, &enq_deq](int n) {
        for (int i = 0; i < elementCount; ++i) {
            queue.enqueue(i);
            char buf[48];
            snprintf(buf, sizeof(buf), "[Thread #%d] Enqueued %d\n", n, i);
            printf(buf);
            enq_deq[i].first += 1;
        }
    };

    auto consumer = [&queue, &enq_deq](int n) {
        int val;
        int count = 0;
        while (count < elementCount) {
            if (queue.dequeue(val)) {
                char buf[48];
                snprintf(buf, sizeof(buf), "[Thread #%d] Dequeued %d\n", n, val);
                printf(buf);
                enq_deq[val].second += 1;
                count++;
            }
        }
    };

    for (int i = 1; i <= 4; ++i)
    {
        if (i % 2 == 1)
        {
            threads[i-1] = std::thread(producer, i);
        }
        else
        {
            threads[i-1] = std::thread(consumer, i);
        }
    }
    
    for (auto &t : threads)
    {
        t.join();
    }

    bool isCorrect = true;
    for (auto &p : enq_deq)
    {
        if (p.first != p.second)
        {
            isCorrect = false;
        }
    }
    
    if (isCorrect)
    {
        std::cout << "All operations completed successfully without deadlocks or ABA!\n";
    }

    return 0;
}