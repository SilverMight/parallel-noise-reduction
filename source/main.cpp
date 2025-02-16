#include "wav_file.hpp"

#include <filesystem>
#include <iostream>
#include <span>
#include <algorithm>
#include <vector>
#include <numbers>

int16_t max = 0; // We will need this variable at the end when we "unnormalize" the audio to match the original vol

void normalizeAudio(std::vector<std::vector<int16_t>>& samples)
{
  // Find max value
  for (auto& channel : samples)
  {
    int16_t channel_max = *std::max_element(channel.begin(), channel.end(), [](int16_t a, int16_t b) { return std::abs(a) < std::abs(b); });
    max = std::max(max, static_cast<int16_t>(std::abs(channel_max)));
  }
    
  // Normalize
  for (auto& channel : samples)
  {
    for (auto& sample : channel)
    {
      sample = static_cast<int16_t>((sample / static_cast<double>(max)) * std::numeric_limits<int16_t>::max());
    }
  }
}

std::vector<int16_t> sumToMono(std::vector<std::vector<int16_t>> samples)
{
  size_t channels = samples.size();
  size_t length_of_samples = samples[0].size();
  std::vector<int16_t> mono_data(length_of_samples);
  for (size_t i = 0; i < length_of_samples; i++)
  {
    int32_t total = 0; // avoid overflow
    for (auto& channel : samples)
    {
      total += channel[i];
    }

    mono_data[i] = static_cast<int16_t>(total / static_cast<int32_t>(channels));
  }
  
  return mono_data;
}

// TODO: double check if this is right
std::vector<std::vector<int16_t>> frameSlice(std::vector<int16_t> samples, size_t frame_size, float overlap_ratio = 0.5f)
{
  std::vector<std::vector<int16_t>> frames;
  size_t overlap = static_cast<size_t>(frame_size * overlap_ratio);
  
  if (overlap_ratio >= 1)
  {
    std::cout << "Error. Overlap ratio is greater than 1." << std::endl;
  }
  
  size_t chunk = frame_size - overlap;
  
  for (size_t i = 0; i < ((samples.size() - overlap)/chunk); i++)
  {
    std::vector<int16_t> frame(samples.begin() + (chunk * i), samples.begin() + (chunk * i) + frame_size);
    frames.push_back(frame);
  }
  return frames;
}


std::vector<double> generateHammingWindow(size_t window_size)
{
  std::vector<double> hamming(window_size);
  for (size_t n = 0; n < window_size; n++)
  {
    hamming[n] = 0.54 - 0.46 * cos(2 * std::numbers::pi * n / window_size - 1);
  }
  
  return hamming;
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
  
  std::cout << "Summing to mono..." << std::endl;
  std::vector<int16_t> mono_data = sumToMono(input_wav.samples);
  
// DEBUG for printing the mono track
//  std::vector<std::vector<int16_t>> mono_wrapper;
//  mono_wrapper.push_back(mono_data);
//  input_wav.samples = mono_wrapper;
//  input_wav.set_num_channels(1);
  
  const auto frame_size = 1024;

  std::vector<std::vector<int16_t>> frames = frameSlice(mono_data, frame_size);
  auto window = generateHammingWindow(frame_size);
  
  // Apply the window function (multiply)
  // this could be parallel...
  for (auto& frame: frames)
  {
    for (size_t i = 0; i < frame_size; i++)
    {
      frame[i] = static_cast<int16_t>(frame[i] * window[i]);
    }
  }
  
  
  
  input_wav.write(output_file);

  return 0;
}

