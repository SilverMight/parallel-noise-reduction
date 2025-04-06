#include <future>
#include <ranges>
#include <vector>
#include <BS_thread_pool.hpp>
#include <fftw3.h>

#include "parallel_audio_processor.hpp"

#include "audio_processing.hpp"

parallel_audio_processor::parallel_audio_processor()
  : parallel_audio_processor({}) {}

parallel_audio_processor::parallel_audio_processor(const options& opts)
    : pool {opts.num_threads}
    , frame_chunking_size {opts.frame_chunking_size}
    , num_noise_frames{opts.num_noise_frames}
    , forward_plan(fftw_plan_dft_r2c_1d(frame_size, nullptr, nullptr, FFTW_ESTIMATE))
    , backward_plan(fftw_plan_dft_c2r_1d(frame_size, nullptr, nullptr, FFTW_ESTIMATE))
{
}

std::vector<std::vector<int16_t>> parallel_audio_processor::process_audio(
    const std::vector<std::vector<int16_t>>& samples)
{
  std::vector<std::vector<int16_t>> cleaned_samples {};

  auto samples_doubles = audio_processing::cast_2d_vec_to_t<double>(samples);

  auto max = audio_processing::normalize_audio(samples_doubles);

  std::vector<std::vector<std::vector<double>>> channel_frames {};

  // do this sequentially, shouldn't take very long, thread overhead may not be
  // worth it.
  for (const auto& channel_samples : samples_doubles) {
    auto frames = audio_processing::frame_slice(channel_samples, frame_size);
    audio_processing::apply_hamming_window(frames);
    channel_frames.push_back(std::move(frames));
  }

  // Calculate noise profile of each thread in parallel
  std::vector<std::vector<double>> channel_noise_profiles =
      get_noise_profiles_threaded(channel_frames);

  // Now, submit async tasks for each channel's frames - we do this so the
  // thread pool receives all chunks of all channels at once
  std::vector<BS::multi_future<std::vector<double>>>
      channels_cleaned_chunks_futures;

  for (auto [channel, channel_noise_profile] : std::views::zip(channel_frames, channel_noise_profiles)) {
        channels_cleaned_chunks_futures.push_back(async_process_channel_chunked(channel, channel_noise_profile, frame_size));
  }

  auto cleaned_channels = std::vector<std::vector<int16_t>>(samples_doubles.size());
  cleaned_channels.reserve(samples_doubles.size());
  for (auto [clean_channel, cleaned_channel_chunks_future] : std::views::zip(cleaned_channels, channels_cleaned_chunks_futures)) {
        //
    // flatten samples
    std::vector<double> result_channel_samples;
    for (auto& future : cleaned_channel_chunks_future) {
      auto result_chunk = future.get();
      result_channel_samples.insert(result_channel_samples.end(),
                                    result_chunk.begin(),
                                    result_chunk.end());
    }

    const auto scaled_samples = audio_processing::scale_samples_and_clamp_to_int16(result_channel_samples, max);

    clean_channel = scaled_samples;
  }

  return cleaned_channels;
}

std::vector<std::vector<double>>
parallel_audio_processor::get_noise_profiles_threaded(const std::vector<std::vector<std::vector<double>>>& channel_frames)
{
  std::vector<std::future<std::vector<double>>> noise_profile_futures;
  noise_profile_futures.reserve(channel_frames.size());

  for (const auto& channel : channel_frames) {
    noise_profile_futures.push_back(
        pool.submit_task([&channel, this]() { return audio_processing::get_noise_profile(channel, num_noise_frames, forward_plan.get()); }));
  }


  std::vector<std::vector<double>> channel_noise_profiles;
  channel_noise_profiles.reserve(noise_profile_futures.size());

  for (auto& future : noise_profile_futures) {
    channel_noise_profiles.push_back(future.get());
  }

  return channel_noise_profiles;
}

// Returns chunks of a given channel, parallelizing their processing
BS::multi_future<std::vector<double>>
parallel_audio_processor::async_process_channel_chunked(const std::vector<std::vector<double>>& channel_frames,
                                                        const std::vector<double>& channel_noise_profile,
                                                        size_t frame_size)
{
  auto chunk_futures = pool.submit_blocks(0,
      channel_frames.size(),
      [&channel_frames, &channel_noise_profile, frame_size, this](
          const std::size_t start, const std::size_t end) {
        const auto frame_chunk = std::vector<std::vector<double>>{channel_frames.begin() + static_cast<std::ptrdiff_t>(start), channel_frames.begin() + static_cast<std::ptrdiff_t>(end)};

        const auto cleaned_frames = audio_processing::spectral_subtraction(frame_chunk, channel_noise_profile, forward_plan.get(), backward_plan.get());
        const auto processed_mono = audio_processing::overlap_add(cleaned_frames, frame_size);

        return processed_mono;
      },
      frame_chunking_size);

  return chunk_futures;
}
