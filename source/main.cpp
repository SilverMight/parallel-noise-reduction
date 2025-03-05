#include "wav_file.hpp"
#include "audio_processing.hpp"

#include <filesystem>
#include <iostream>
#include <span>
#include <algorithm>
#include <vector>
#include <numbers>


auto main(int argc, char *argv[]) -> int {
  if (argc < 3) {
    std::cout << "Usage: Executable [input_file.wav] [output_file.wav]\n";
    return -1;
  }

  const auto args = std::span(argv, static_cast<size_t>(argc));

  const std::filesystem::path input_file{args[1]};
  const std::filesystem::path output_file{args[2]};

  if (!std::filesystem::exists(input_file)) {
    std::cout << "Input file " << input_file.string() << "does not exist.\n";
    return -1;
  }

  wav_file input_wav{input_file};
  
  const auto new_samples = audio_processing::process_audio(input_wav.samples);

  // temp method for printing file
  input_wav.samples = new_samples;

  input_wav.write(output_file);

  return 0;
}

