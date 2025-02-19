#include "audio_processing.hpp"

#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <vector>
namespace audio_processing {

namespace {
template<typename T>
std::vector<std::vector<T>> cast_2d_vec_to_t(const std::vector<std::vector<int16_t>>& input) {
    std::vector<std::vector<T>> output{};
    output.reserve(input.size());

    for (const auto& channel : input) {
        std::vector<T> float_samples{};
        float_samples.reserve(channel.size());


        for (const auto sample : channel) {
            float_samples.push_back(static_cast<T>(sample));
        }

        output.push_back(std::move(float_samples));
    }

    return output;
}
}  // namespace

int16_t normalize_audio(std::vector<std::vector<double>>& samples)
{
  int16_t max{};
  // Find max value
  for (const auto& channel : samples)
  {
    const auto channel_max = std::max_element(channel.begin(), channel.end(), [](int16_t a, int16_t b) { return std::abs(a) < std::abs(b); });

    // TODO: check empty vec here
    max = std::max(max, static_cast<int16_t>(std::abs(*channel_max)));
  }

  // Normalize
  for (auto& channel : samples)
  {
    for (auto& sample : channel)
    {
      sample /= max;
      sample *= std::numeric_limits<int16_t>::max();
    }
  }

  // Return the max value out for use later.
  return max;
}

std::vector<double> sum_to_mono(const std::vector<std::vector<double>>& samples)
{
  const size_t channels = samples.size();
  const size_t length_of_samples = samples[0].size();

  std::vector<double> mono_data(length_of_samples);

  for (size_t i = 0; i < length_of_samples; i++)
  {
    double total = 0; // avoid overflow
    for (const auto& channel : samples)
    {
      total += channel[i];
    }

    mono_data[i] = total / static_cast<double>(channels);
  }

  return mono_data;
}

// TODO: double check if this is right
std::vector<std::vector<double>> frame_slice(const std::vector<double>& samples, size_t frame_size, double overlap_ratio = default_overlap)
{
  std::vector<std::vector<double>> frames;
  const auto overlap = static_cast<size_t>(static_cast<double>(frame_size) * overlap_ratio);

  // TODO: handle this properly
  assert(overlap_ratio < 1);

  const auto chunk = frame_size - overlap;
  const auto num_frames = ((samples.size() - overlap)/chunk);

  for (size_t i = 0; i < num_frames; i++)
  {
    const auto start = chunk * i;
    auto frame = std::vector(samples.begin() + static_cast<ptrdiff_t>(start),
                             samples.begin() + static_cast<ptrdiff_t>(start + frame_size));
    frames.push_back(std::move(frame));
  }
  return frames;
}


std::vector<double> generate_hamming_window(size_t window_size)
{
  std::vector<double> hamming(window_size);

  constexpr auto coefficient = 0.54;
  for (size_t n = 0; n < window_size; n++)
  {
    hamming[n] = coefficient - (1 - coefficient) * std::cos((2 * std::numbers::pi * static_cast<double>(n)) / static_cast<double>(window_size - 1));
  }

  return hamming;
}

void apply_hamming_window(std::vector<std::vector<double>>& frames) {
  const auto hamming_window_constants = generate_hamming_window(frames.size());


  for(auto& frame: frames) {
    for(size_t sample_idx = 0; sample_idx < frame.size(); sample_idx++) {
      frame[sample_idx] *= hamming_window_constants[sample_idx];
    }
  }
  
}

std::vector<std::vector<int16_t>> process_audio(const std::vector<std::vector<int16_t>>& samples) {
  // Cast to double
  auto samples_doubles = cast_2d_vec_to_t<double>(samples);

  auto max = normalize_audio(samples_doubles);
  
  auto mono_data = sum_to_mono(samples_doubles);
  
// DEBUG for printing the mono track
//  std::vector<std::vector<int16_t>> mono_wrapper;
//  mono_wrapper.push_back(mono_data);
//  input_wav.samples = mono_wrapper;
//  input_wav.set_num_channels(1);
  
  const auto frame_size = 1024;

  auto frames = frame_slice(mono_data, frame_size);
  apply_hamming_window(frames);
  
  
  // TODO: Actually return samples!
  return {};  
}
}  // namespace audio_processing
