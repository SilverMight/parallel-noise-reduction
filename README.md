# parallel-noise-reduction

parallel-noise-reduction uses spectral subtraction to process audio files and reduce noise in parallel.

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Usage

```bash
./build/parallel-noise-reduction [OPTIONS] input-file.wav output-file.wav
```

# Benchmark
A [prepared set of WAV files containing noise](https://drive.google.com/drive/folders/1S3Tb6UfNnOkwKGDTBBVp-IH55mMGy2GM) is provided to showcase the performance of parallel-noise-reduction.

## Options
* `-h, --help`: print help message
* `--threads`: Number of threads to use while processing audio. Default is number of threads in system.
* `--noise-frames`: Number of frames to count as noise frames while analyzing the audio.

# Current Bugs
* Small WAV files may not behave well or cause crashes.
* WAV files with headers not defined against the [typical format](http://soundfile.sapp.org/doc/WaveFormat/) standard may crash the program.
