#include <algorithm>
#include <filesystem>
#include <future>
#include <iostream>
#include <numbers>
#include <ranges>
#include <span>
#include <vector>

#include <CLI/CLI.hpp>

#include "audio_processing.hpp"
#include "parallel_audio_processor.hpp"
#include "wav_file.hpp"

auto main(int argc, char* argv[]) -> int
{
  CLI::App app{"Parallel Noise Reducer"};

  std::filesystem::path input_file {};
  std::filesystem::path output_file {};

  app.add_option("input-file", input_file, "File to process.")->required();
  app.add_option("output-file", output_file, "Silenced output file.")->required();

  parallel_audio_processor::options opts{};
  app.add_option("--threads", opts.num_threads, "Number of threads to use while processing audio. Default is number of threads in system.")->capture_default_str();

  app.add_option("--noise-frames", opts.num_noise_frames, "Number of frames to count as noise frames when analyzing audio")->capture_default_str();
  // TODO: support specifying chunk size


  CLI11_PARSE(app, argc, argv);

  if (!std::filesystem::exists(input_file)) {
    std::cout << "Input file " << input_file.string() << "does not exist.\n";
    return -1;
  }

  wav_file input_wav {input_file};

  parallel_audio_processor processor{opts};

  const auto cleaned_samples = processor.process_audio(input_wav.samples);

  input_wav.samples = cleaned_samples;
  input_wav.write(output_file);

  return 0;
}


