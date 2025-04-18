#include <cstdint>

#include <BS_thread_pool.hpp>
#include "fftw_memory.hh"


class parallel_audio_processor
{
public:
    // Options to instantiate a parallel audio processor with.
    struct options {
        size_t num_threads = std::thread::hardware_concurrency();
        size_t frame_chunking_size = 32;
        size_t num_noise_frames = 50;
    };

    explicit parallel_audio_processor();
    explicit parallel_audio_processor(const options& opts);

    // Main function to process audio with
    std::vector<std::vector<int16_t>> process_audio(
        const std::vector<std::vector<int16_t>>& samples);

private:
    // Threaded function to get noise profiles for all channels simultaneously
    // Returns back 2D array with noise profile for each channel.
    std::vector<std::vector<double>> get_noise_profiles_threaded(
        const std::vector<std::vector<std::vector<double>>>& channel_frames);

    // Process a given channels frames 
    BS::multi_future<std::vector<double>> async_process_channel_chunked(
        const std::vector<std::vector<double>>& channel_frames,
        const std::vector<double>& channel_noise_profile);

    std::vector<int16_t> process_frames(const std::vector<double>& mono_data,
                                        int16_t max);

    static constexpr size_t frame_size = 1024;

    BS::thread_pool<BS::tp::none> pool;
    std::size_t frame_chunking_size;
    std::size_t num_noise_frames;

    fftw_memory::fftw_plan_unique_ptr forward_plan;
    fftw_memory::fftw_plan_unique_ptr backward_plan;
};
