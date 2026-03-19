# Succinct Filesystem

FUSE filesystem based on FLOUDS and other succinct data structures

## Features

- Structure of the filesystem using FLOUDS
- Different bitvector implementations
- Different name sequence strategies
- Block device abstraction
- Different allocation strategies
- File system manager for persistence and metadata management
- Benchmarking suite for performance evaluation

## Requirements

- C++20
- CMake (3.14)
- Git
- For mounting the filesystem on Linux: libfuse
- GTest
- Best performance on modern processors with BMI2 support

### Installing

#### macOS (Testing only, FUSE not supported)
```bash
brew install git cmake googletest
```

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install git cmake libfuse2 libgtest-dev
```

## Usage

### 1. Clone with Submodules

```bash
git clone --recurse-submodules https://github.com/sebastianbrunnert/succinct-filesystem
```

Or if you already cloned the repository:

```bash
git submodule update --init --recursive
```

### 2. Build the Project

```bash
make build
```

### 3. Running Tests

You can run the tests using CTest:

```bash
make test
```

### 4. Linking

Using FUSE, you can mount the filesystem to the folder `tmp` with image file `tmp.img`:

```bash
make link
```

Own parameters can be defined by running the FUSE-daemon directly:

```bash
./build/succinct_filesystem other.img other
```

## License

Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>

This project is licensed under the GPL-2.0-only License.

## Author

Sebastian Brunnert - [mail@sebastianbrunnert.de](mailto:mail@sebastianbrunnert.de)

This project is part of my bachelor thesis at the University of Münster. For more information, please refer to the thesis document.