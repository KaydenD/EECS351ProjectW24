# Real-Time FM Demodulation Project

## Overview
This program is designed for real-time demodulation of FM radio signals using an RTL-SDR. It captures live IQ data, processes these through a demodulation pipeline, and outputs the audio to system sound devices. Developed in C++ utilizing the `rtlsdrlib` libary from osmocom for RTL-SDR interaction, `RtAudio` for outputting audio and mangaing audio devices, and `moodycamel::ReaderWriterQueue` as a queue between the thread.

## Project Structure
- **`/fm-demod`**:
  - `CMakeLists.txt` - Configuration for building the FM demodulation module.
  - `fm-demod.cpp` - Implementation of FM demodulation algorithms.
  - `fm-demod.hpp` - Header for demodulation functions.
  
- **`/rtl-sdr-helpers`**:
  - `CMakeLists.txt` - Configuration for building the RTL-SDR helpers.
  - `rtl-sdr-helpers.cpp` - Helper functions to interface with RTL-SDR hardware.
  - `rtl-sdr-helpers.hpp` - Header for RTL-SDR helper functions.
  
- **`CMakeLists.txt`** - Main CMake configuration for the project.
- **`main.cpp`** - Main application entry, orchestrating data capture to audio output.
- **`vcpkg.json`** - Manifest file specifying project dependencies.

## Functionality
The RTL-SDR captures IQ data which is fed into the `FmDemod` class. This class processes the IQ data to extract audio using FM demodulation techniques. The resulting audio is played back in real-time, allowing live streaming of FM channels.

## Compilation and Setup
### Requirements:
- **CMake** - For building the project.
- **vcpkg** - For managing dependencies.
- **Visual Studio** - Recommended for its integrated support with CMake and vcpkg.
