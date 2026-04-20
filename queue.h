#include <mutex>
#include <queue>

template <typename T>
class BlockingQueue {
    std::queue<T> q;
    std::mutex mtx;
public:
    void enqueue(const T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(value);
    }
    bool dequeue(T& result) {
        std::lock_guard<std::mutex> lock(mtx);
        if (q.empty()) return false;
        result = q.front();
        q.pop();
        return true;
    }
};