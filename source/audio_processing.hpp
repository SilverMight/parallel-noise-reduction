#pragma once

#include <cstdint>
#include <vector>
namespace audio_processing {
constexpr auto default_overlap = 0.5;

int16_t normalize_audio(std::vector<std::vector<double>>& samples);

std::vector<double> sum_to_mono(const std::vector<std::vector<double>>& samples);

// TODO: double check if this is right
std::vector<std::vector<double>> frame_slice(const std::vector<double>& samples, size_t frame_size, double overlap_ratio);


std::vector<double> generate_hamming_window(size_t window_size);
void apply_hamming_window(std::vector<std::vector<double>>& frames, size_t window_size);

std::vector<double> get_noise_profile(const std::vector<std::vector<double>>& frames);

std::vector<double> overlap_add(const std::vector<std::vector<double>>& frames,  size_t frame_size, double overlap_ratio);

std::vector<int16_t> scale_samples_and_clamp_to_int16(const std::vector<double>& normalized_mono_samples, int16_t max);

std::vector<std::vector<int16_t>> process_audio(const std::vector<std::vector<int16_t>>& samples);

}  // namespace audio_processing
