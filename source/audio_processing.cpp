#include "audio_processing.hpp"

#include "fftw3.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <ranges>
#include <span>
#include <numbers>
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

inline double complex_magnitude_squared(fftw_complex& complex) {
  return (complex[0] * complex[0]) + (complex[1] * complex[1]);
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

std::vector<std::vector<double>> frame_slice(const std::vector<double>& samples, size_t frame_size, double overlap_ratio = default_overlap)
{
  std::vector<std::vector<double>> frames;
  const auto overlap = static_cast<size_t>(static_cast<double>(frame_size) * overlap_ratio);

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
  const auto frame_size = frames[0].size();
  const auto hamming_window_constants = generate_hamming_window(frame_size);


  for(auto& frame: frames) {
    for(size_t sample_idx = 0; sample_idx < frame_size; sample_idx++) {
      frame[sample_idx] *= hamming_window_constants[sample_idx];
    }
  }
  
}

std::vector<vector<double>> spectral_subtraction(const std::vector<std::vector<double>>& frames,
                                                 const std::vector<double>& noise_profile) {

  
}

std::vector<double> get_noise_profile(const std::vector<std::vector<double>>& frames) {
  constexpr size_t noise_frames = 5;

  const auto frame_size = frames[0].size();
  const auto complex_size = frame_size / 2 + 1;


  auto *fft_in = fftw_alloc_real(frame_size);
  auto *fft_out = fftw_alloc_complex(complex_size);

  auto fft_in_span = std::span{fft_in, frame_size};
  auto fft_out_span = std::span{fft_out, complex_size};

  fftw_plan forward_plan = fftw_plan_dft_r2c_1d(static_cast<int>(frame_size),
                                                fft_in,
                                                fft_out, 
                                                FFTW_ESTIMATE);


  // Noise profile calculation
  std::vector<double> noise_profile(frame_size/2 + 1, 0.0);
  const auto num_noise_frames = std::min(noise_frames, frames.size());

  for(std::size_t i = 0; i < num_noise_frames; ++i) {
    std::copy(frames[i].begin(), frames[i].end(), fft_in);
    fftw_execute(forward_plan);

    for(auto [noise_frame, fft_frame] : std::views::zip(noise_profile, fft_out_span)) {
      noise_frame += std::sqrt(complex_magnitude_squared(fft_frame));
    }
  }

  // Average noise frames.
  for(auto& val : noise_profile) {
    val /= static_cast<double>(num_noise_frames);
  }

  // todo: consider using smart pointers or the like..
  fftw_destroy_plan(forward_plan);
  fftw_free(fft_in);
  fftw_free(fft_out);

  return noise_profile;
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

  const auto noise_profile = get_noise_profile(frames);
  
  
  // TODO: Actually return samples!
  return {};  
}
}  // namespace audio_processing
