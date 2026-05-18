# Project Overview
Name: main_project
Tech stack: C++20, Qt 6.11.1, CUDA 13.2, CMake 4.3.2, GCC 16.1.1
Compute capability: 8.6
CUDA Toolkit path: /opt/cuda
Qt installation prefix: /usr

# Build & Test Commands
## Configure
mkdir -p build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr -DCMAKE_BUILD_TYPE=Debug

## Build
cmake --build . --config Debug -j $(nproc)

## Run tests
ctest --output-on-failure

## Run application
./build/__EXECUTABLE_NAME__

# Architecture
- Core logic: `src/core/` (pure C++, no Qt)
- CUDA kernels: `src/cuda/` (each .cu file maps to one high‑level operation)
- Qt UI: `src/ui/` (Widgets + .ui files)
- Common header: `include/main_project/`
- Tests: `tests/` (Google Test)

# Coding Conventions
- Naming: classes/structs UpperCamelCase, functions snake_case, variables camelCase
- Error handling: use `cuda_check(err)` macro; Qt methods return bool or use exceptions
- Memory: std::unique_ptr for ownership; std::shared_ptr only when truly shared
- Documentation: Doxygen comments on all public API

# GPU Guidelines
- Default block size: 256 threads
- Use shared memory for repeated accesses within a block
- Always check `cudaGetLastError()` after kernels
- Preferred profiling tool: nvprof / Nsight Systems

# Known Issues / TODOs
- (list any current pain points or planned features)