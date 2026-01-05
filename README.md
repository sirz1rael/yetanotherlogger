# Header-Only Logger (Pro)

A professional-grade, lightweight, thread-safe, header-only C++17 logging library with Sink architecture, source location tracking, **File Rotation**, and **Sanitizer Support**.

## Key Features

- **Header-only**: Zero external dependencies (only standard C++17).
- **Sink Architecture**: Easily log to console, files, or custom destinations.
- **File Rotation**: Limit file size and number of log files automatically.
- **Thread-safe**: Mutex-protected logging for multi-threaded applications.
- **Source Location**: Captures file name, line number, and function name.
- **Quality Ensured**: Built-in support for ASAN and Valgrind checks.

---

## Technical Features & Debugging

### AddressSanitizer (ASAN)
The library automatically enables ASAN in `Debug` builds to catch memory errors early.
To build with ASAN:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_ASAN=ON
```

### Valgrind
A dedicated target is available to run memory leak checks using Valgrind:
```bash
cmake --build . --target valgrind-check
```

---

## File Logging & Configuration

### Rotating File Sink (Limits)
```cpp
// Parameters: filename, max_file_size_bytes, max_files_to_keep
auto rotating = std::make_shared<Logger::RotatingFileSink>(
    "logs/app.log", 
    10 * 1024 * 1024, // 10 MB
    5                 // Keep 5 old log files
);
Logger::Logger::instance().add_sink(rotating);
```

---

## Integration

### Via CMake FetchContent
```cmake
include(FetchContent)
FetchContent_Declare(
    header_only_logger
    GIT_REPOSITORY https://github.com/YOUR_USERNAME/header_only_logger.git
    GIT_TAG main
)
FetchContent_MakeAvailable(header_only_logger)

target_link_libraries(your_target PRIVATE header_only_logger::logger)
```

## Testing
```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
make
ctest
# Run memory check
make valgrind-check
```

## License
MIT License.
