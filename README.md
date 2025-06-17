# OSCAR (Renderer)
**OS**cilloscope **C**ode **A**nd **R**enderer

This is the rendering component of the OSCAR live coding environment, designed for live performances with oscilloscope visuals. It emulates a four-channel oscilloscope, where each channel takes in a left and right audio input and plots it on an XY plane.

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
- `./src/build/oscar_render`