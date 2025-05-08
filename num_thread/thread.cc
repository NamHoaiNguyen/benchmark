#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <numeric>

int run_benchmark_once() {
    using namespace std::chrono;
    auto start_time = high_resolution_clock::now();
    auto end_time = start_time + seconds(1);

    int created = 0;

    while (high_resolution_clock::now() < end_time) {
        std::thread t([]{}); // Minimal thread function
        t.join();
        created++;
    }

    return created;
}

int main() {
    const int runs = 10;
    std::vector<int> results;

    for (int i = 0; i < runs; ++i) {
        int count = run_benchmark_once();
        results.push_back(count);
        std::cout << "Run " << (i + 1) << ": " << count << " threads created/destroyed\n";
    }

    double average = std::accumulate(results.begin(), results.end(), 0.0) / runs;
    std::cout << "\nAverage over " << runs << " runs: " << average << " threads/second\n";

    return 0;
}
