#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

long benchmark_fork() {
  pid_t pid;
  long count{0};

  auto start_time = std::chrono::high_resolution_clock::now();
  while (true) {
    pid = ::fork();
    if (pid == -1)
    {
      auto current_time = std::chrono::high_resolution_clock::now();
      std::chrono::seconds elapsed =
          std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);

      if (elapsed.count() > 1) {
        break;
      } else {
        continue;
      }
    }

    if (pid == 0) {
      std::exit(0);
    } else {
      ::wait(NULL);
      count++;
    }
    
    
    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::seconds elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);

    if (elapsed.count() > 1) {
      break;
    }
  }

  return count;
}

void Benchmark(int num_tests) {
  long total = 0;

  for (int i = 0; i < num_tests; i++) {
    long res = benchmark_fork();
    std::cout << "Trial " << i + 1 << ": " << res <<
        " processes created and destroyed in 1 second." << std::endl;
    total += res;
  }

  std::cout << "\n\n\n";
  std::cout << "================================================================";

  // Calculate average result
  double average = static_cast<double>(total) / num_tests;
  std::cout << "\nAverage number of processes created and destroyed per second: " << average << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "Usage " << "<exec file> " << " <number of times testing>(less than 20)" << std::endl;
    return 0;
  }

  int num_tests = static_cast<int>(std::stoi(argv[2]));
  if (num_tests <= 0 || num_tests > 20) {
    return 0;
  }

  Benchmark(num_tests);

  return 0;
}