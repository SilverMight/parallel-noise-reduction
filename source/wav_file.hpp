#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

/**
 * @brief The core implementation of parallel noise reduction
 *
 *
 */

class wav_file {
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

  } wav_header{};

  void validate_header();

public:
  explicit wav_file(const std::filesystem::path &file_path);
};
