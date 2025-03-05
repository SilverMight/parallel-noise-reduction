#include "audio_processing.hpp"

#include <fftw3.h>

#include "fftw_memory.hh"

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

inline double complex_magnitude(fftw_complex& complex) {
  return std::sqrt((complex[0] * complex[0]) + (complex[1] * complex[1]));
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

std::vector<std::vector<double>> spectral_subtraction(const std::vector<std::vector<double>>& frames,
                                                 const std::vector<double>& noise_profile) {
  using fftw_memory::fftw_unique_ptr;

  std::vector<std::vector<double>> clean_frames; 

  const auto frame_size = frames[0].size();
  const auto complex_size = frame_size / 2 + 1;

  using fftw_memory::make_fftw_unique;
  using fftw_memory::fftw_plan_unique_ptr;

  auto fft_in = make_fftw_unique<double>(frame_size);
  auto fft_out = make_fftw_unique<fftw_complex>(complex_size);

  auto fft_in_span = std::span{fft_in.get(), 
                               frame_size};
  auto fft_out_span = std::span{fft_out.get(),
                                complex_size};

  auto forward_plan = fftw_plan_unique_ptr{fftw_plan_dft_r2c_1d(static_cast<int>(frame_size),
                                                                fft_in.get(),
                                                                fft_out.get(), 
                                                                FFTW_ESTIMATE)};

  auto cleaned_ifft_in = make_fftw_unique<double>(frame_size);
  auto cleaned_ifft_in_span  = std::span{cleaned_ifft_in.get(), frame_size};

  auto backward_plan = fftw_plan_unique_ptr{fftw_plan_dft_c2r_1d(static_cast<int>(frame_size),
                                                                 fft_out.get(),
                                                                 cleaned_ifft_in.get(), 
                                                                 FFTW_ESTIMATE)};

  // Perform spectral subtraction.
  for(const auto& frame : frames) {

    std::copy(frame.begin(), frame.end(), fft_in.get());

    fftw_execute(forward_plan.get());


    for(auto [noise_frame, fft_frame] : std::views::zip(noise_profile, fft_out_span)) {
      double& real = fft_frame[0];
      double& imag = fft_frame[1];

      auto mag = complex_magnitude(fft_frame);
      auto phase = std::atan2(imag, real);

      // clamp to 0 if we get a negative value
      double subtracted_mag = std::max(0.0, mag - noise_frame);

      real = subtracted_mag * std::cos(phase);
      imag = subtracted_mag * std::sin(phase);

      // Do IFFT back to reals.
    }

    fftw_execute(backward_plan.get());

    for(auto &ifft_frame : cleaned_ifft_in_span) {
      ifft_frame /= static_cast<double>(frame_size);
    }

    clean_frames.emplace_back(cleaned_ifft_in_span.begin(), cleaned_ifft_in_span.end());
  }

  return clean_frames;
}

std::vector<double> get_noise_profile(const std::vector<std::vector<double>>& frames) {
  constexpr size_t noise_frames = 5;

  const auto frame_size = frames[0].size();
  const auto complex_size = frame_size / 2 + 1;

  using fftw_memory::make_fftw_unique;
  using fftw_memory::fftw_unique_ptr;
  using fftw_memory::fftw_plan_unique_ptr;

  auto fft_in = make_fftw_unique<double>(frame_size);
  auto fft_out = make_fftw_unique<fftw_complex>(complex_size);

  auto fft_in_span = std::span{fft_in.get(), frame_size};
  auto fft_out_span = std::span{fft_out.get(), complex_size};

  auto forward_plan = fftw_plan_unique_ptr{fftw_plan_dft_r2c_1d(static_cast<int>(frame_size),
                                                                fft_in.get(),
                                                                fft_out.get(), 
                                                                FFTW_ESTIMATE)};


  // Noise profile calculation
  std::vector<double> noise_profile(frame_size/2 + 1, 0.0);
  const auto num_noise_frames = std::min(noise_frames, frames.size());

  for(std::size_t i = 0; i < num_noise_frames; ++i) {
    std::copy(frames[i].begin(), frames[i].end(), fft_in.get());
    fftw_execute(forward_plan.get());

    for(auto [noise_frame, fft_frame] : std::views::zip(noise_profile, fft_out_span)) {
      noise_frame += complex_magnitude(fft_frame);
    }
  }

  // Average noise frames.
  for(auto& val : noise_profile) {
    val /= static_cast<double>(num_noise_frames);
  }

  return noise_profile;
}

std::vector<double> overlap_add(const std::vector<std::vector<double>>& frames,  size_t frame_size, double overlap_ratio = default_overlap)
{
  const auto overlap = static_cast<size_t>(static_cast<double>(frame_size) * overlap_ratio);

  const auto hop = frame_size - overlap; // This is the size of the frame ignoring the overlapped section

  // Find the total length needed for output (assuming mono...)
  // (n-1) "hop" frames + a full frame
  // TODO: figure out the correct formula for this
  const auto output_size = (hop * (frames.size() - 1) + frame_size);

  // Initialize output array
  std::vector<double> output(output_size, 0.0);

  // we also need to calculate the weights (over total array) to unweight them
  std::vector<double> weight_sum(output_size, 0.0);

  const auto hamming_window_constants = generate_hamming_window(frame_size);

  // NOTE: there is a way to do this more efficiently using the fact that the hamming window is repeated but im lazy (and the beginning and tail ends wont have the same weight pattern)

  // add each frame to the output buffer
  for (std::size_t i = 0; i < frames.size(); ++i)
  {
    const auto absolute_index = i * hop;

    for (std::size_t j = 0; j < frame_size && j < frames[i].size(); ++j)
    {
      output[absolute_index + j] += frames[i][j];
      weight_sum[absolute_index + j] += hamming_window_constants[j];
    }
  }

  // unweight
  for (std::size_t i = 0; i < output_size; ++i)
  {
    assert(weight_sum[i] > 0.0);
    output[i] /= weight_sum[i];
  }

  return output;
}

std::vector<std::vector<int16_t>> process_audio(const std::vector<std::vector<int16_t>>& samples) {
  // Cast to double
  auto samples_doubles = cast_2d_vec_to_t<double>(samples);

  auto max = normalize_audio(samples_doubles);
  
  auto cleaned_channels = std::vector<std::vector<int16_t>>{};

  cleaned_channels.reserve(samples_doubles.size());

  for(const auto& mono_data: samples_doubles) {
    // DEBUG for printing the mono track
    //  std::vector<std::vector<int16_t>> mono_wrapper;
    //  mono_wrapper.push_back(mono_data);
    //  input_wav.samples = mono_wrapper;
    //  input_wav.set_num_channels(1);

    const auto frame_size = 1024;

    auto frames = frame_slice(mono_data, frame_size);
    apply_hamming_window(frames);

    const auto noise_profile = get_noise_profile(frames);

    const auto cleaned_frames = spectral_subtraction(frames, noise_profile);
    const auto processed_mono = overlap_add(cleaned_frames, frame_size);

    // Convert back to int16_t with proper scaling
    std::vector<int16_t> result;
    result.reserve(processed_mono.size());

    // Scale to use the int16_t range
    double scale = max > 0 ? static_cast<double>(std::numeric_limits<int16_t>::max()) / max : 1.0;

    for (const auto& sample : processed_mono) {
      // cast to int (int32_t to avoid overflow)
      int32_t scaled_sample = static_cast<int32_t>(std::round(sample * scale));

      // check to int16_t range
      scaled_sample = std::max(scaled_sample, static_cast<int32_t>(std::numeric_limits<int16_t>::min()));
      scaled_sample = std::min(scaled_sample, static_cast<int32_t>(std::numeric_limits<int16_t>::max()));
      result.push_back(static_cast<int16_t>(scaled_sample));
    }

    cleaned_channels.push_back(std::move(result));
  }

  fftw_cleanup();

  return cleaned_channels;
}
}  // namespace audio_processing
