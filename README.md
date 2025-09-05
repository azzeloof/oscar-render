# OSCAR (Renderer)
**OS**cilloscope **C**ode **A**nd **R**enderer

[![C++ Build](https://github.com/azzeloof/oscar-render/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/azzeloof/oscar-render/actions/workflows/c-cpp.yml)

This is the rendering component of the OSCAR live coding environment, designed for live performances with oscilloscope visuals. It emulates a four-channel oscilloscope, where each channel takes in a left and right audio input and plots it on an XY plane.

This may be used standalone to visualize any audio signals, or in concert with the [OSCAR Language](https://github.com/azzeloof/oscar-language) and [VSCode Plugin](https://github.com/azzeloof/oscar-vscode) to form a complete live coding environment.

Below is some sparse documentation to help you get started with OSCAR.

---
## Features
- Four channels (easy to extend it to add more)
- OSC interface to control rendering parameters (color, thickness, etc) for each channel
- Cross-platform architecture (although it hasn't been built for windows yet)

---
## Building
Clone the repo, then run `git submodule update --init` and enter the `src` directory.

### Linux

1.  Install the required libraries using your package manager. For example, on Debian/Ubuntu:

    ```bash
    sudo apt-get install libasio-dev libjack-dev
    ```
    > **⚠️ Important SFML Requirement:** This project requires **SFML 3.0** or newer. Many distributions, including Ubuntu, provide an older version of SFML (2.x) in their default package managers (`libsfml-dev`). Until this is updated, you will need to download the SFML 3.0 SDK manually.
    >
    > You can get the latest Linux SDK from the [official SFML download page](https://www.sfml-dev.org/download/sfml/3.0.0/). After downloading and extracting, you will need to copy the `include` and `lib` files to a system directory like `/usr/local/`.

2.  Build the project:

    ```bash
    make
    ```

### macOS (Apple Silicon)

1.  Install Homebrew if you don't have it already.
2.  Install the required libraries:

    ```bash
    brew install sfml asio jack qjackctl
    ```

3.  Build the project:

    ```bash
    make
    ```

### Windows

Building on Windows has not been tested yet, but it should be possible. If you get this working please open a PR.

---
## Usage

To launch the program, run `./src/build/oscar_render`. It will create a virtual audio device that can be viewed and patched to using a tool like qjackctl or qpwgraph. To change the display parameters, use OSC messages on port 7000:
 - `/scope/n/trace/thickness/x.x` (float, generally 0.0 - 10.0, trace thickness in pixels)
 - `/scope/n/persistence/samples/x` (integer, generally 100 - 30000, number of samples to display to emulate phosphor glow effect)
 - `/scope/n/persistence/strength/x` (integer, 0 - 255, opacity of phosphor glow effect)
 - `/scope/n/trace/color/x` (integer, packed RGBA)
 - `/scope/n/trace/blur/x.x` (float, generally 0.0 - thickness/2, blur radius in pixels)
 - `/scope/n/alpha_scale/x.x` (float, 0.0 - 1.0)
 - `/scope/n/scale/x`
