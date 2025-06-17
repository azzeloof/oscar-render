# OSCAR (Renderer)
**OS**cilloscope **C**ode **A**nd **R**enderer

This is the rendering component of the OSCAR live coding environment, designed for live performances with oscilloscope visuals. It emulates a four-channel oscilloscope, where each channel takes in a left and right audio input and plots it on an XY plane.

Below is some sparse documentation to help you get started with OSCAR.

## Features
- Four channels (easy to extend it to add more)
- OSC interface to control rendering parameters (color, thickness, etc) for each channel
- Cross-platform architecture (although it has only actually been built on Linux so far)

## Building (Linux)
There are already hooks in the Makefile to build for Mac as well, but is untested. Any help in documenting the build process for Mac or Windows would be appreciated!

- Clone the repo
- Install the SFML libraries for your OS
- Pull the libraries: `git submodule update --init`
- `cd src && make`

## Usage

To launch the program, run `./src/build/oscar_render` and select the audio device you'd like to listen to in the terminal. You can use a virtual audio device (JACK, VB-Cable, etc) to connect the output from another program to the OSCAR input. To change the display parameters, use OSC messages on port 7000:
 - `/scope/n/trace/thickness/x.x` (float, generally 0.0 - 10.0, trace thickness in pixels)
 - `/scope/n/persistence/samples/x` (integer, generally 100 - 30000, number of samples to display to emulate phosphor glow effect)
 - `/scope/n/persistence/strength/x` (integer, 0 - 255, opacity of phosphor glow effect)
 - `/scope/n/trace/color/x` (integer, packed RGBA)
 - `/scope/n/trace/blur/x.x` (float, generally 0.0 - thickness/2, blur radius in pixels)
 - `/scope/n/alpha_scale/x.x` (float, 0.0 - 1.0)
 - `/scope/n/scale/x`