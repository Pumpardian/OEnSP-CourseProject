#include "stand.h"

int main(int argc, char* argv[]) {
    if (argc != 7) {
        std::cerr << "Usage: " << argv[0] 
                  << " <producers_count> <consumers_count> <ops_per_thread> <load_ops> <load_threads> <load_push_ratio>\n"
                  << "All arguments must be positive integers.\n"
                  << "producers_count, producers_count and load_threads are limited to 100.\n"
                  << "load_push_ratio is a value between 0 and 100 (exclusive).\n"
                  << "Example: " << argv[0] << " 4 4 10000 200000 16 50\n";
        return 1;
    }

    int producers_count, consumers_count, ops_per_thread, load_ops, load_threads, load_push_ratio;

    try {
        producers_count = std::stoi(argv[1]);
        consumers_count = std::stoi(argv[2]);
        ops_per_thread  = std::stoi(argv[3]);
        load_ops        = std::stoi(argv[4]);
        load_threads    = std::stoi(argv[5]);
        load_push_ratio    = std::stoi(argv[6]);

        if (producers_count <= 0 || consumers_count <= 0 ||
            ops_per_thread <= 0 || load_ops <= 0 || load_threads <= 0
            || producers_count > 100 || consumers_count > 100 || load_threads > 100
            || load_push_ratio >= 100 || load_push_ratio <= 0) {
            throw std::invalid_argument("Invalid arguments");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: Invalid arguments.\n"
                  << "Usage: " << argv[0] 
                  << " <producers_count> <consumers_count> <ops_per_thread> <load_ops> <load_threads> <load_push_ratio>\n"
                  << "All arguments must be positive integers.\n"
                  << "producers_count, producers_count and load_threads are limited to 100.\n"
                  << "load_push_ratio is a value between 0 and 100 (exclusive).\n"
                  << "Example: " << argv[0] << " 4 4 10000 200000 16 50\n";
        return 1;
    }

    std::cout << "=== Starting test stand ===\n\n";
    run_functional_test(producers_count, consumers_count, ops_per_thread);
    run_load_test<BlockingQueue<int>>("Blocking Queue", load_threads, load_ops, load_push_ratio);
    run_load_test<LockFreeQueue<int>>("Lock-Free Queue", load_threads, load_ops, load_push_ratio);

    return 0;
}