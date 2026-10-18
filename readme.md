# Schemer

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CI](https://github.com/Kacper-Jakubiak/Schemer/actions/workflows/ci.yml/badge.svg)](https://github.com/Kacper-Jakubiak/Schemer/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/Kacper-Jakubiak/Schemer)](https://github.com/Kacper-Jakubiak/Schemer/releases)

A modern, high-performance, C++ library.

---

## Features

- **Fast & Lightweight:** Designed with zero-overhead principles and minimal dependencies.
- **Easy Integration:** Supports CMake `add_subdirectory`, `FetchContent`, and find_package.

---

## Table of Contents

- [Requirements](#requirements)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Documentation](#documentation)
- [Building and Running Tests](#building-and-running-tests)
- [License](#license)

---

## Requirements

To build and use this library, you need:

- A C++20 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.21 or higher

---

## Installation

Schemer is header-only. Include `Schemer.h` (or an individual public header) and
link the CMake interface target `Schemer::Schemer` to inherit the Eigen
dependency and C++20 requirement; no Schemer library binary needs to be built
or distributed.

### Option 1: CMake `FetchContent` (Recommended)

Add the following to your `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
    Schemer
    GIT_REPOSITORY https://github.com/Kacper-Jakubiak/Schemer.git
    GIT_TAG v1.0.0 # or a specific commit/branch
)
FetchContent_MakeAvailable(Schemer)

target_link_libraries(your_target PRIVATE Schemer::Schemer)

```

### Option 2: Manual / Submodule

1. Clone or add the repository as a submodule in your project's `vendor/` directory:
   ```bash
   git submodule add https://github.com/Kacper-Jakubiak/Schemer.git vendor/Schemer
   ```
2. In your `CMakeLists.txt`:
   ```cmake
   add_subdirectory(vendor/Schemer)
   target_link_libraries(your_target PRIVATE Schemer::Schemer)
   ```

---

## Quick Start

Here is a simple example of how to use the library:

```cpp
#include <iostream>

int main() {

}
```

---

## Documentation

Full documentation, API reference, and advanced tutorials can be found
at [https://Kacper-Jakubiak.github.io/Schemer](https://Kacper-Jakubiak.github.io/Schemer).

---

## Building and Running Tests

To build the project along with its unit tests locally:

```bash
# Clone the repository
git clone [https://github.com/Kacper-Jakubiak/Schemer.git](https://github.com/Kacper-Jakubiak/Schemer.git)
cd Schemer

# Create build directory
mkdir build && cd build

# Configure with tests enabled
cmake -DPROJECT_BUILD_TESTS=ON ..

# Build
cmake --build .

# Run tests (using CTest)
ctest --output-on-failure
```

---

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.