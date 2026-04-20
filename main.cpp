#include "stand.h"

int main(int argc, char* argv[]) {
    std::cout << "=== Starting test stand ===\n\n";

    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] 
                  << " <producers_count> <producers_count> <ops_per_thread> <load_ops> <load_threads>\n"
                  << "All arguments must be positive integers.\n"
                  << "producers_count, producers_count and load_threads are limited to 100.\n"
                  << "Example: " << argv[0] << " 4 4 10000 200000 16\n";
        return 1;
    }

    int producers_count, consumers_count, ops_per_thread, load_ops, load_threads;

    try {
        producers_count = std::stoi(argv[1]);
        consumers_count = std::stoi(argv[2]);
        ops_per_thread  = std::stoi(argv[3]);
        load_ops        = std::stoi(argv[4]);
        load_threads    = std::stoi(argv[5]);

        if (producers_count <= 0 || consumers_count <= 0 ||
            ops_per_thread <= 0 || load_ops <= 0 || load_threads <= 0
            || producers_count > 100 || consumers_count > 100 || load_threads > 100) {
            throw std::invalid_argument("All arguments must be positive integers");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: Invalid arguments.\n"
                  << "Usage: " << argv[0] 
                  << " <producers_count> <consumers_count> <ops_per_thread> <load_ops> <load_threads>\n"
                  << "All arguments must be positive integers.\n"
                  << "producers_count, producers_count and load_threads are limited to 100.\n"
                  << "Example: " << argv[0] << " 4 4 10000 200000 16\n";
        return 1;
    }

    run_functional_test(producers_count, consumers_count, ops_per_thread);

    run_load_test<BlockingQueue<int>>("Blocking Queue", 1, 1, load_ops);
    run_load_test<LockFreeQueue<int>>("Lock-Free Queue", 1, 1, load_ops);

    run_load_test<BlockingQueue<int>>("Blocking Queue", load_threads, load_threads, load_ops);
    run_load_test<LockFreeQueue<int>>("Lock-Free Queue", load_threads, load_threads, load_ops);

    return 0;
}