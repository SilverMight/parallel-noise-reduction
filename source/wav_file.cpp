#include "wav_file.hpp"

#include <fmt/format.h>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <bit>
#include <array>
#include <cstring>
#include <cstdint>
#include <vector>

wav_file::wav_file(const std::filesystem::path &file_path) {
  // Open file stream
  std::ifstream file{file_path, std::ios::binary};

  // read first part of WAV header
  file.read(reinterpret_cast<char *>(&wav_header), sizeof(wav_header));

  // Validate the WAV header
  validate_header();

  // Skip past wav header.
  file.seekg(sizeof(wav_header));

  // The "data" chunk is not guaranteed to be the second chunk.
  // We seek over the file to find the data chunk.
  char chunk_id[4];
  uint32_t chunk_size{};

  std::vector<char> raw_audio_data;

  bool found_data = false;
  while(file.read(chunk_id, sizeof(chunk_id))) {
    file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size));


    // Found data!
    if(std::string{chunk_id, 4} == "data") {
      found_data = true;

      // Resize audio data vector to fit bytes and read data into it
      raw_audio_data.resize(chunk_size);
      file.read(reinterpret_cast<char*>(raw_audio_data.data()), chunk_size);
      break;

    } 

    // Otherwise continue seeking.
    file.seekg(chunk_size, std::ios::cur);
  }

  if(!found_data) {
    throw std::runtime_error("Did not find data chunk!");
  }

  read_samples(raw_audio_data);
}

void wav_file::read_samples(const std::vector<char>& raw_audio_data) {
  // Given that chunk size == NumSamples * NumChannels * BitsPerSample/8
  const size_t bytes_per_sample = (wav_header.bits_per_sample / 8);

  // Can only handle 16-bit samples right now
  // TODO: Handle this nicely
  assert(sizeof(int16_t) == bytes_per_sample);

  const auto num_samples = raw_audio_data.size() / (bytes_per_sample * wav_header.num_channels );

  samples = std::vector<std::vector<int16_t>>
    {wav_header.num_channels, std::vector<int16_t>(num_samples)}; 

  for(size_t i = 0; i < num_samples; ++i) {
    for(size_t ch = 0; ch < wav_header.num_channels; ++ch) {
      auto index = (i * wav_header.num_channels + ch) * bytes_per_sample;

      int16_t sample{};

      // Need to reinterpret char to int16_t
      std::memcpy(&sample, &raw_audio_data[index], sizeof(int16_t));

      samples[ch][i] = sample;
    }
  }

}

std::vector<char> wav_file::get_raw_data_from_samples() const {
  const size_t bytes_per_sample = (wav_header.bits_per_sample / 8);
  const auto num_samples = samples[0].size();
  const auto num_channels = samples.size();

  std::vector<char> raw_audio_data(num_samples * num_channels * bytes_per_sample);
  assert(bytes_per_sample == 2);

  for(size_t ch = 0; ch < wav_header.num_channels; ++ch) {
    for(size_t i = 0; i < num_samples; ++i) {
      auto index = (i * wav_header.num_channels + ch) * bytes_per_sample;

      const auto sample = samples[ch][i];
      const auto sample_bytes = 
        std::bit_cast<std::array<char, sizeof(int16_t)>>(sample);

        raw_audio_data[index] = sample_bytes[0];
        raw_audio_data[index+1] = sample_bytes[1];
    }
  }

  return raw_audio_data;
}


void wav_file::validate_header() const {
  // "RIFF" ASCII string
  const std::string chunk_id{wav_header.chunk_id, 4};

  if (chunk_id != "RIFF") {
    throw std::runtime_error(fmt::format("Invalid chunk ID, got {}", chunk_id));
  }

  // "WAVE" ASCII string
  const std::string format{wav_header.format, 4};
  if (format != "WAVE") {
    throw std::runtime_error(fmt::format("Invalid format, got {}", format));
  }

  const std::string subchunk_1_id{wav_header.subchunk_1_id, 4};
  if (subchunk_1_id != "fmt ") {
    throw std::runtime_error(
        fmt::format("Invalid subchunk 1 ID, got {}", subchunk_1_id));
  }
}

void wav_file::write(const std::filesystem::path& file_path) const {
  // Revalidate header
  validate_header(); 

  // Open file stream
  std::ofstream file{file_path, std::ios::binary};

  file.write(reinterpret_cast<const char*>(&wav_header), sizeof(wav_header));

  file.seekp(sizeof(wav_header));

  const auto raw_audio_data = get_raw_data_from_samples();

  constexpr char chunk_id[] = {'d', 'a', 't', 'a'};
  const auto chunk_size = static_cast<uint32_t>(raw_audio_data.size());

  file.write(chunk_id, sizeof(chunk_id));
  file.write(reinterpret_cast<const char *>(&chunk_size), sizeof(chunk_size));

  file.write(raw_audio_data.data(), chunk_size);

  file.close();
}

uint16_t wav_file::get_num_channels() const {
  return wav_header.num_channels;
}

void wav_file::set_num_channels(uint16_t num)
{
  wav_header.num_channels = num;
}
