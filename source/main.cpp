#include "wav_file.hpp"

#include <filesystem>
#include <iostream>
#include <span>
#include <algorithm>
#include <vector>
#include <cmath>

int16_t max = 0; // We will need this variable at the end when we "unnormalize" the audio to match the original vol

void normalizeAudio(std::vector<std::vector<int16_t>>& samples)
{
  // Find max value
  for (auto& channel : samples)
  {
    int16_t channel_max = *std::max_element(channel.begin(), channel.end(), [](int16_t a, int16_t b) { return std::abs(a) < std::abs(b); });
    max = std::max(max, static_cast<int16_t>(std::abs(channel_max)));
  }
  
  std::cout << "The max value is " << max << std::endl;
  
  // Normalize
  for (auto& channel : samples)
  {
    for (auto& sample : channel)
    {
      sample = static_cast<int16_t>((sample / static_cast<float>(max)) * INT16_MAX);
    }
  }
}

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

  wav_file input_wav{input_file};
  
  normalizeAudio(input_wav.samples);
  
  // Window function (this is just an array with weights that we apply to "frames" of our signal. https://en.wikipedia.org/wiki/Window_function#Hann_and_Hamming_windows
  // TODO: we need to find what a good frame size is AND we need to figure out our windowing function. The 2 that stand out are Hann and Hamming
  
  input_wav.write(output_file);

  return 0;
}
