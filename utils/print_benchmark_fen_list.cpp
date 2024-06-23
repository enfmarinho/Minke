#include "../src/benchmark.h"
#include <iostream>

int main(int argc, char *argv[]) {
  for (std::string line : benchmark_fen_list) {
    std::cout << line << '\n';
  }
  std::cout << std::flush;
  return 0;
}
