# stdui

`stdui` is an experimental modern declarative UI framework for C++.

[![CI](https://github.com/maidamai0/stdui/actions/workflows/ci.yml/badge.svg)](https://github.com/maidamai0/stdui/actions/workflows/ci.yml)
[![Coverage](https://github.com/maidamai0/stdui/actions/workflows/coverage.yml/badge.svg)](https://github.com/maidamai0/stdui/actions/workflows/coverage.yml)
[![Docs](https://github.com/maidamai0/stdui/actions/workflows/docs.yml/badge.svg)](https://github.com/maidamai0/stdui/actions/workflows/docs.yml)

> **Warning:** `stdui` is under heavy development and is not ready for production
> use. The public API and behavior may change without notice.

The current public surface is header-only and targets C++20.

## Build

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Install

```sh
cmake --install build --prefix /path/to/prefix
```

The installed package can be consumed with CMake:

```cmake
find_package(stdui CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE stdui::stdui)
```

## Coverage

Coverage reporting currently requires Clang and LLVM tools.

```sh
cmake -S . -B build -DSTDUI_ENABLE_COVERAGE=ON
cmake --build build
cmake --build build --target stdui_coverage
```
