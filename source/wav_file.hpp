#pragma once

#include <filesystem>
#include <vector>

class wav_file {
public:
  explicit wav_file(const std::filesystem::path &file_path);
  void write(const std::filesystem::path &file_path) const;

  const std::vector<std::vector<int16_t>>& get_samples();
  void set_samples(std::vector<std::vector<int16_t>> new_samples);
  
private:
  void validate_header() const;
  void read_samples(const std::vector<char>& raw_audio_data);
  std::vector<char> get_raw_data_from_samples() const;

  // Defined at http://soundfile.sapp.org/doc/WaveFormat/
  struct {
    // RIFF chunk descriptor
    char chunk_id[4];
    uint32_t chunk_size;
    char format[4];

    // fmt sub chunk
    char subchunk_1_id[4];
    uint32_t subchunk_1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
  } wav_header {};
  // Contains actual audio samples in the format
  // samples[channels][samples]
  std::vector<std::vector<int16_t>> samples;
  
};
