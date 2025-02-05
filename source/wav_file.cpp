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
