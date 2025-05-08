#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <chrono>
#include <numeric>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;
using namespace std::chrono;

int64_t benchmark_read_once(const char* filename, size_t buffer_size) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    std::vector<char> buffer(buffer_size);
    ssize_t bytes_read{0}, bytes{0};

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1 || !S_ISREG(file_stat.st_mode)) {
      std::exit(1);
      // return 0;
    }
    
    size_t file_size = file_stat.st_size;

    auto start = high_resolution_clock::now();

    while (bytes_read < file_size) {
      bytes = ::read(fd, buffer.data(), buffer_size);
      if (bytes == 0) { // EOF
        break;
      }

      if (bytes == -1) {
        std::exit(1);
        return 0;
      }

      bytes_read += bytes;
    }


    auto end = high_resolution_clock::now();
    close(fd);

    int64_t time_execute = duration_cast<milliseconds>(end - start).count();

    return time_execute;
}


void benchmark_read_avg(const char* filename, size_t buffer_size, int runs = 10) {
    std::vector<int64_t> durations;

    for (int i = 0; i < runs; ++i) {
        int64_t time_ms = benchmark_read_once(filename, buffer_size);
        if (time_ms >= 0) durations.push_back(time_ms);
    }

    if (durations.empty()) {
        std::cerr << "Failed to benchmark with buffer size: " << buffer_size << "\n";
        return;
    }

    double avg = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
    std::cout << "Buffer size: " << buffer_size << " bytes -> Average Time: "
              << avg << " ms over " << durations.size() << " runs\n";
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    const char* filename = argv[1];
    std::vector<size_t> buffer_sizes =
        {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
         4096, 8192, 16384, 32768, 65536, 262144, 524288, 1048576};

    for (auto size : buffer_sizes) {
        benchmark_read_avg(filename, size);
    }

    return 0;
}