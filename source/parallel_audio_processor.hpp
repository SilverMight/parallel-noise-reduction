#include <cstdint>

#include <BS_thread_pool.hpp>

class parallel_audio_processor
{
public:
  struct options
  {
    size_t num_threads = std::thread::hardware_concurrency();
    size_t frame_chunking_size = 32;
    size_t num_noise_frames = 50;
  };

  explicit parallel_audio_processor();
  explicit parallel_audio_processor(const options& opts);

  std::vector<std::vector<int16_t>> process_audio(
      const std::vector<std::vector<int16_t>>& samples);

  std::vector<std::vector<double>> get_noise_profiles_threaded(
      const std::vector<std::vector<std::vector<double>>>& channel_frames);

  BS::multi_future<std::vector<double>> async_process_channel_chunked(
      const std::vector<std::vector<double>>& channel_frames,
      const std::vector<double>& channel_noise_profile,
      size_t frame_size);

  std::vector<int16_t> process_frames(const std::vector<double>& mono_data,
                                      int16_t max);

private:
  BS::thread_pool<BS::tp::none> pool;
  std::size_t frame_chunking_size;
  std::size_t num_noise_frames;
};
