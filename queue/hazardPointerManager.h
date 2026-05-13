#include <vector>
#include <thread>
#include <algorithm>
#include "node.h"

constexpr int MAX_THREADS = 200;
constexpr int RETIRE_THRESHOLD = 100;

template <typename T>
class HazardPointerManager {
    std::atomic<void*> hazard_pointers[MAX_THREADS][2];
    
    struct ThreadState {
        int id = -1;
        std::vector<Node<T>*> retire_list;

        ~ThreadState() { for (auto ptr : retire_list) delete ptr; } 
    };

    static thread_local ThreadState local_state;
    std::atomic<int> thread_counter{0};

    int get_thread_id() {
        if (local_state.id == -1) {
            local_state.id = thread_counter.fetch_add(1);
        }
        return local_state.id;
    }

public:
    HazardPointerManager() {
        for (int i = 0; i < MAX_THREADS; ++i) {
            hazard_pointers[i][0].store(nullptr);
            hazard_pointers[i][1].store(nullptr);
        }
    }

    void protect(int index, Node<T>* node) {
        int tid = get_thread_id();
        hazard_pointers[tid][index].store(node, std::memory_order_release);
    }

    void clear(int index) {
        int tid = get_thread_id();
        hazard_pointers[tid][index].store(nullptr, std::memory_order_release);
    }

    void retire(Node<T>* node) {
        local_state.retire_list.push_back(node);
        if (local_state.retire_list.size() >= RETIRE_THRESHOLD) {
            scan();
        }
    }

    void scan() {
        std::vector<void*> active_hps;
        active_hps.reserve(MAX_THREADS * 2);

        for (int i = 0; i < MAX_THREADS; ++i) {
            void* p0 = hazard_pointers[i][0].load(std::memory_order_acquire);
            if (p0) active_hps.push_back(p0);
            
            void* p1 = hazard_pointers[i][1].load(std::memory_order_acquire);
            if (p1) active_hps.push_back(p1);
        }

        std::sort(active_hps.begin(), active_hps.end());

        std::vector<Node<T>*> new_retire_list;
        for (Node<T>* node : local_state.retire_list) {
            if (std::binary_search(active_hps.begin(), active_hps.end(), node)) {
                new_retire_list.push_back(node);
            } else {
                delete node; 
            }
        }
        local_state.retire_list = std::move(new_retire_list);
    }
};

template <typename T>
thread_local typename HazardPointerManager<T>::ThreadState HazardPointerManager<T>::local_state = {-1, {}};