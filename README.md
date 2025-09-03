# OSCAR (Renderer)
**OS**cilloscope **C**ode **A**nd **R**enderer

This is the rendering component of the OSCAR live coding environment, designed for live performances with oscilloscope visuals. It emulates a four-channel oscilloscope, where each channel takes in a left and right audio input and plots it on an XY plane.

Below is some sparse documentation to help you get started with OSCAR.

## Features
- Four channels (easy to extend it to add more)
- OSC interface to control rendering parameters (color, thickness, etc) for each channel
- Cross-platform architecture (although it has only actually been built on Linux so far)

## Building

### macOS (Apple Silicon)

1.  Install Homebrew if you don't have it already.
2.  Install the required libraries:

    ```bash
    brew install sfml asio
    ```

3.  Build the project:

    ```bash
    make
    ```

### Linux

1.  Install the required libraries using your package manager. For example, on Debian/Ubuntu:

    ```bash
    sudo apt-get install libsfml-dev libasio-dev
    ```

2.  Build the project:

    ```bash
    make
    ```

### Windows

Building on Windows has not been tested yet, but it should be possible. You will need to install SFML and Asio and configure the Makefile accordingly.

## Usage

To launch the program, run `./src/build/oscar_render` and select the audio device you'd like to listen to in the terminal. You can use a virtual audio device (JACK, VB-Cable, etc) to connect the output from another program to the OSCAR input. To change the display parameters, use OSC messages on port 7000:
 - `/scope/n/trace/thickness/x.x` (float, generally 0.0 - 10.0, trace thickness in pixels)
 - `/scope/n/persistence/samples/x` (integer, generally 100 - 30000, number of samples to display to emulate phosphor glow effect)
 - `/scope/n/persistence/strength/x` (integer, 0 - 255, opacity of phosphor glow effect)
 - `/scope/n/trace/color/x` (integer, packed RGBA)
 - `/scope/n/trace/blur/x.x` (float, generally 0.0 - thickness/2, blur radius in pixels)
 - `/scope/n/alpha_scale/x.x` (float, 0.0 - 1.0)
 - `/scope/n/scale/x`
