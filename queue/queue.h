#include "hazardPointerManager.h"
#include <memory>

template <typename T>
class LockFreeQueue {
    std::atomic<Node<T>*> head;
    std::atomic<Node<T>*> tail;
    HazardPointerManager<T> hp_manager;

public:
    LockFreeQueue() {
        Node<T>* dummy = new Node<T>();
        head.store(dummy);
        tail.store(dummy);
    }

    ~LockFreeQueue() {
        T dummy_val;
        while (dequeue(dummy_val));
        delete head.load();
    }

    void enqueue(const T& value) {
        Node<T>* new_node = new Node<T>(value);

        while (true) {
            Node<T>* tail_node = tail.load(std::memory_order_acquire);

            hp_manager.protect(0, tail_node);

            if (tail.load(std::memory_order_acquire) != tail_node) continue;

            Node<T>* next_node = tail_node->next.load(std::memory_order_acquire);
            
            if (tail.load(std::memory_order_acquire) != tail_node) continue;

            if (next_node != nullptr) {
                tail.compare_exchange_weak(tail_node, next_node, std::memory_order_release, std::memory_order_relaxed);
                continue;
            }

            Node<T>* expected_null = nullptr;
            if (tail_node->next.compare_exchange_weak(expected_null, new_node, std::memory_order_release, std::memory_order_relaxed)) {
                tail.compare_exchange_weak(tail_node, new_node, std::memory_order_release, std::memory_order_relaxed);

                hp_manager.clear(0);
                return;
            }
        }
    }

    bool dequeue(T& result) {
        while (true) {
            Node<T>* head_node = head.load(std::memory_order_acquire);

            hp_manager.protect(0, head_node);
            if (head.load(std::memory_order_acquire) != head_node) continue;

            Node<T>* tail_node = tail.load(std::memory_order_acquire);
            Node<T>* next_node = head_node->next.load(std::memory_order_acquire);
            
            hp_manager.protect(1, next_node);
            if (head.load(std::memory_order_acquire) != head_node) continue;

            if (head_node == tail_node) {
                if (next_node == nullptr) {
                    hp_manager.clear(0);
                    hp_manager.clear(1);
                    return false;
                }
                tail.compare_exchange_weak(tail_node, next_node, std::memory_order_release, std::memory_order_relaxed);
                continue;
            }

            if (next_node == nullptr) continue;

            result = next_node->data;

            if (head.compare_exchange_weak(head_node, next_node, std::memory_order_release, std::memory_order_relaxed)) {
                hp_manager.retire(head_node);

                hp_manager.clear(0);
                hp_manager.clear(1);
                return true;
            }
        }
    }
};