#include <iostream>
#include <fstream>
#include <functional>
#include <vector>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <numeric>

#include <string.h>

using namespace std;
using namespace std::chrono;

void ClearPageCache()
{
  int fd;
  char buf[2];
  const char *drop_caches_path = "/proc/sys/vm/drop_caches";

  fd = open(drop_caches_path, O_WRONLY);
  if (fd == -1)
  {
    std::cout << "Error opening /proc/sys/vm/drop_caches" << std::endl;
    return; // Return an error code
  }

  ::strcpy(buf, "3");
  if (write(fd, buf, strlen(buf)) != strlen(buf))
  {
    std::cout << "Error writing to /proc/sys/vm/drop_caches" << std::endl;
    close(fd);
    return;
  }

  // Close the file descriptor
  if (close(fd) == -1)
  {
    std::cout << "Error closing /proc/sys/vm/drop_caches" << std::endl;
    return;
  }

  // std::cout << "Page cache cleared successfully." << std::endl;
  return;
}

// Get file size
size_t GetFileSize(const char *filename)
{
  struct stat st;
  if (stat(filename, &st) != 0)
  {
    perror("stat");
    return 0;
  }
  return st.st_size;
}

// Single-threaded read benchmark
long long BenchmarkRead(const char *filename, size_t buffer_size)
{
  int fd = open(filename, O_RDONLY);
  if (fd < 0)
  {
    perror("open");
    return -1;
  }

  vector<char> buffer(buffer_size);
  ssize_t bytes_read{0};
  size_t file_size = GetFileSize(filename);
  ssize_t bytes{0};

  auto start = high_resolution_clock::now();

  while (bytes_read < file_size)
  {
    bytes = ::read(fd, buffer.data(), buffer_size);
    if (bytes == 0)
    {
      break;
    }

    if (bytes == -1)
    {
      std::exit(1);
    }

    bytes_read += bytes;
  }

  auto end = high_resolution_clock::now();
  close(fd);

  ClearPageCache();

  return duration_cast<milliseconds>(end - start).count();
}

// Multithreaded pread benchmark
long long BenchmarkPread(const char *filename, size_t buffer_size, int num_threads)
{
  size_t file_size = GetFileSize(filename);
  if (file_size == 0)
    return -1;

  int fd = open(filename, O_RDONLY);
  if (fd < 0)
  {
    perror("open");
    return -1;
  }

  auto start = high_resolution_clock::now();

  vector<thread> threads;
  size_t chunk_size = file_size / num_threads;

  for (int i = 0; i < num_threads; ++i)
  {
    threads.emplace_back([=]() {
      size_t offset = i * chunk_size;
      size_t size = (i == num_threads - 1) ? file_size - offset : chunk_size;
      vector<char> local_buf(buffer_size);
      ssize_t bytes{0}, bytes_read{0};

      while (bytes_read < size) {
        size_t to_read = std::min(buffer_size, size - bytes_read);
        bytes = pread(fd, local_buf.data(), to_read, offset);
        if (bytes == 0) {
          break;
        }

        if (bytes == -1) {
          std::exit(1);
        }

        bytes_read += bytes;
      }
    });
  }

  for (auto &t : threads)
    t.join();

  auto end = high_resolution_clock::now();
  close(fd);

  ClearPageCache();

  return duration_cast<milliseconds>(end - start).count();
}

// Run benchmark N times and return average
double AverageBenchmark(std::function<long long()> func, int runs = 10)
{
  vector<long long> results;
  for (int i = 0; i < runs; ++i)
  {
    long long t = func();
    if (t > 0)
      results.push_back(t);
  }
  return accumulate(results.begin(), results.end(), 0.0) / results.size();
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    cerr << "Usage: " << argv[0] << " <filename> << " << "<buffersize>(bytes unit) "
                                                         "\n";
    return 1;
  }

  const char *filename = argv[1];

  size_t buffer_size = static_cast<size_t>(std::stoul(argv[2]));

  const int num_threads = thread::hardware_concurrency(); // Or fixed number

  cout << "Benchmarking read (single-thread)...\n";
  double avg_read = AverageBenchmark([&]()
                                     { return BenchmarkRead(filename, buffer_size); });
  cout << "Average time (read, 1 thread): " << avg_read << " ms\n\n";

  cout << "Benchmarking pread (multi-thread, " << num_threads << " threads)...\n";
  double avg_pread = AverageBenchmark([&]()
                                      { return BenchmarkPread(filename, buffer_size, num_threads); });
  cout << "Average time (pread, " << num_threads << " threads): " << avg_pread << " ms\n";

  return 0;
}
