#include "wav_file.hpp"
#include <filesystem>
#include <fmt/format.h>
#include <stdexcept>

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

  read_raw_audio_data();
}

void wav_file::read_raw_audio_data() {
  // Given that chunk size == NumSamples * NumChannels * BitsPerSample/8
  const size_t bytes_per_sample = (wav_header.bits_per_sample / 8);
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

void wav_file::validate_header() {
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

uint16_t wav_file::get_num_channels() const {
  return wav_header.num_channels;
}
