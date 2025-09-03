# Oscar Render

A simple oscilloscope renderer.

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

After building, you can run the executable:

```bash
./build/oscar_render
```
