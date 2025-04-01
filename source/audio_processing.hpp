#pragma once

#include <cstdint>
#include <vector>
namespace audio_processing {
constexpr auto default_overlap = 0.5;

template<typename T, typename U>
std::vector<std::vector<T>> cast_2d_vec_to_t(const std::vector<std::vector<U>>& input) {
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

int16_t normalize_audio(std::vector<std::vector<double>>& samples);

std::vector<double> sum_to_mono(const std::vector<std::vector<double>>& samples);

// TODO: double check if this is right
std::vector<std::vector<double>> frame_slice(const std::vector<double>& samples, size_t frame_size, double overlap_ratio = default_overlap);


std::vector<double> generate_hamming_window(size_t window_size);
void apply_hamming_window(std::vector<std::vector<double>>& frames);

std::vector<double> get_noise_profile(const std::vector<std::vector<double>>& frames, std::size_t num_noise_frames);

std::vector<std::vector<double>> spectral_subtraction(const std::vector<std::vector<double>>& frames,
                                                      const std::vector<double>& noise_profile); 

std::vector<double> overlap_add(const std::vector<std::vector<double>>& frames,  size_t frame_size, double overlap_ratio = default_overlap);

std::vector<int16_t> scale_samples_and_clamp_to_int16(const std::vector<double>& normalized_mono_samples, int16_t max);

}  // namespace audio_processing
