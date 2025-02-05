#include "wav_file.hpp"

#include <filesystem>
#include <iostream>
#include <span>

auto main(int argc, char *argv[]) -> int {
  if (argc < 3) {
    std::cout << "Usage: executable [input_file.wav] [output_file.wav]\n";
    return -1;
  }

  const auto args = std::span(argv, static_cast<size_t>(argc));

  const std::filesystem::path input_file{args[1]};
  const std::filesystem::path output_file{args[2]};

  if (!std::filesystem::exists(input_file)) {
    std::cout << "Input file " << input_file.string() << "does not exist.\n";
    return -1;
  }

  const wav_file input_wav{input_file};

  return 0;
}
