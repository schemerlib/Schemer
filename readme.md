# Schemer

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Latest release](https://img.shields.io/github/v/release/schemerlib/Schemer)](https://github.com/schemerlib/Schemer/releases)

Schemer is a header-only C++ library for simulating one-dimensional fractional
diffusion. It provides a builder-based API for configuring a simulation, running
time steps, and saving results.

## Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Releases](#releases)
- [Quick start](#quick-start)
- [Configuration](#configuration)
- [Building and running tests](#building-and-running-tests)
- [License](#license)

## Features

- Fractional diffusion with an order parameter `alpha` in `(0, 2)`.
- Configurable skewness, scale, spatial grid, and time-stepping scheme.
- Constant or position-dependent forces, with centered and upwind force schemes.
- Optional stochastic resetting.
- State access and result or simulation-history output.

## Requirements

- A C++20-compatible compiler.
- CMake 3.21 or later.
- Git and network access when CMake first downloads Eigen (and doctest when tests
  are enabled).

Eigen is fetched by CMake and linked through the `Schemer::Schemer` interface
target.

## Installation

Schemer is header-only; there is no library binary to build or install. Include
the public header and use the CMake interface target or provide the required
include paths and C++20 setting to your compiler.

### CMake FetchContent

Add Schemer to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
    Schemer
    GIT_REPOSITORY https://github.com/schemerlib/Schemer.git
    GIT_TAG master
)
FetchContent_MakeAvailable(Schemer)

target_link_libraries(your_target PRIVATE Schemer::Schemer)
```

### Manual

Download and extract the [Eigen 3.4.0 release](https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.zip),
and the [Schemer release](https://github.com/schemerlib/Schemer/releases).

```sh
git clone https://github.com/schemerlib/Schemer.git
```

Both Schemer and Eigen are header-only, so there are no library binaries to
link. Pass the directory containing `Schemer/` and the directory containing
`Eigen/` to `g++` as include paths:

```sh
g++ -std=c++20 \
    -I/path/to/Schemer \
    -I/path/to/eigen-3.4.0 \
    your_source.cpp -o your_program
```

Replace the paths and source filename with the locations on your system.

## Quick start

```cpp
#include <Eigen/Dense>
#include <Schemer/Schemer.h>

int main() {
    Schemer::Builder builder;
    builder.set_alpha(1.5)
           .set_num_intervals(200)
           .set_length(20.0)
           .set_dt(0.01)
           .set_log_interval_percent(0.0);

    Schemer::Runner runner = builder.build();
    runner.run(1000);

    const Eigen::VectorXd values = runner.get_values();
    runner.save_result("result.dat");
}
```

The grid contains `num_intervals + 1` points. `get_values()` returns the state
at the current time step, and `save_result()` writes the configuration and
current state to a file. Progress logging is enabled by default; set its
interval to `0.0` to disable it.

## Configuration

`Schemer::Builder` supports method chaining to configure the simulation:

- `set_alpha()`, `set_beta()`, and `set_sigma()` control the fractional
  diffusion.
- `set_length()`, `set_num_intervals()`, and `set_dt()` configure the spatial
  grid and time step.
- `set_drift()` applies a constant force. The `set_force()` overloads accept a
  force function or a vector with one value per grid point.
- `set_initial_conditions()` accepts custom initial values.
- `set_reset_rate()` and the reset-index methods configure stochastic resetting.
- `set_theta()` selects the time-stepping scheme; `0.0` is explicit, `0.5` is
  semi-implicit, and `1.0` is implicit.

See the public headers for the complete API: [Builder](Schemer/Builder.h),
[Runner](Schemer/Runner.h), and [configuration types](Schemer/Config.h).

## Building and running tests

Tests use doctest 2.4.11, which CMake downloads automatically when
`BUILD_TESTING` is enabled. Eigen 3.4.0 is also downloaded automatically.
No separate dependency installation is needed, but the initial CMake
configuration requires Git and network access.

Configure and build the project with tests enabled, then run CTest:

```sh
git clone https://github.com/schemerlib/Schemer.git
cd Schemer
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## License

Schemer is distributed under the MIT License. See [LICENSE](LICENSE) for the
full license text.
