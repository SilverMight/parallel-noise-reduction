#pragma once

#include <cstdint>
#include <vector>
#include <fftw3.h>
namespace audio_processing {
constexpr auto default_overlap = 0.5;

// Utility function to cast a 2D vec of type U to type T
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

// Audio normalization
int16_t normalize_audio(std::vector<std::vector<double>>& samples);

// Overlapping frame slice (samples -> frames)
std::vector<std::vector<double>> frame_slice(const std::vector<double>& samples, size_t frame_size, double overlap_ratio = default_overlap);
// Overlap add (frames -> samples)
std::vector<double> overlap_add(const std::vector<std::vector<double>>& frames,  size_t frame_size, double overlap_ratio = default_overlap);


// Hamming window functions
std::vector<double> generate_hamming_window(size_t window_size);
void apply_hamming_window(std::vector<std::vector<double>>& frames);

// Noise profile estimation
std::vector<double> get_noise_profile(const std::vector<std::vector<double>>& frames, std::size_t num_noise_frames, fftw_plan forward_plan);

// Spectral subtraction
std::vector<std::vector<double>> spectral_subtraction(const std::vector<std::vector<double>>& frames,
                                                      const std::vector<double>& noise_profile,
                                                      fftw_plan forward_plan,
                                                      fftw_plan backward_plan); 


// Scaling of samples to denormalize them & clamping back to int16_t
std::vector<int16_t> scale_samples_and_clamp_to_int16(const std::vector<double>& normalized_mono_samples, int16_t max);

}  // namespace audio_processing
