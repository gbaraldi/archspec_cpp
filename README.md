# archspec_cpp

A C++ library for CPU microarchitecture detection and feature querying, inspired by the Python [archspec](https://github.com/archspec/archspec) library. This library is designed to be embedded directly in larger projects (like the Julia language) for integration with LLVM and other compilers.

## Features

- **CPU Detection**: Automatically detect the host CPU microarchitecture
- **Feature Querying**: Check for specific CPU features (AVX, AVX2, AVX-512, NEON, SVE, etc.)
- **Compiler Flags**: Get optimal compiler optimization flags for GCC, Clang, and other compilers
- **Cross-Platform**: Supports Linux, macOS, FreeBSD, and Windows
- **Architectures**: x86_64, AArch64, PPC64/PPC64LE, RISC-V 64
- **Header-Only JSON**: Uses nlohmann/json for JSON parsing
- **Embedded Data**: CPU database is embedded in the library for easy distribution

## Requirements

- C++17 compatible compiler
- [nlohmann/json](https://github.com/nlohmann/json) header-only library (included in parent directory)

## Building

```bash
# Build the library and tests
make

# Run tests
make check

# Run examples
make run-examples

# Debug build
make debug

# Clean
make clean
```

## Usage

### Basic Host Detection

```cpp
#include <archspec/archspec.hpp>
#include <iostream>

int main() {
    // Detect the host CPU
    archspec::Microarchitecture host = archspec::host();
    
    std::cout << "CPU: " << host.name() << std::endl;
    std::cout << "Vendor: " << host.vendor() << std::endl;
    std::cout << "Family: " << host.family() << std::endl;
    
    // Check for specific features
    if (host.has_feature("avx2")) {
        std::cout << "AVX2 is supported!" << std::endl;
    }
    
    return 0;
}
```

### Getting Compiler Flags

```cpp
#include <archspec/archspec.hpp>
#include <iostream>

int main() {
    // Get a specific microarchitecture
    const auto* haswell = archspec::get_target("haswell");
    
    if (haswell) {
        // Get optimization flags for GCC 10.0
        std::string flags = haswell->optimization_flags("gcc", "10.0");
        std::cout << "GCC flags: " << flags << std::endl;
        // Output: GCC flags: -march=haswell -mtune=haswell
    }
    
    return 0;
}
```

### Feature Checking

```cpp
#include <archspec/archspec.hpp>
#include <iostream>

int main() {
    archspec::Microarchitecture host = archspec::host();
    
    // Check various features
    std::vector<std::string> features_to_check = {
        "sse", "sse2", "avx", "avx2", "avx512f", "fma"
    };
    
    for (const auto& feature : features_to_check) {
        std::cout << feature << ": " 
                  << (host.has_feature(feature) ? "yes" : "no") 
                  << std::endl;
    }
    
    return 0;
}
```

### Iterating All Known Targets

```cpp
#include <archspec/archspec.hpp>
#include <iostream>

int main() {
    const auto& db = archspec::MicroarchitectureDatabase::instance();
    
    for (const auto& [name, target] : db.all()) {
        std::cout << name << " (" << target.vendor() << ")" << std::endl;
    }
    
    return 0;
}
```

## API Reference

### Main Classes

#### `archspec::Microarchitecture`

Represents a CPU microarchitecture.

```cpp
class Microarchitecture {
    // Accessors
    const std::string& name() const;
    const std::string& vendor() const;
    const std::set<std::string>& features() const;
    int generation() const;  // For POWER CPUs
    const std::string& cpu_part() const;  // For ARM CPUs
    
    // Feature checking
    bool has_feature(const std::string& feature) const;
    
    // Ancestry
    std::vector<std::string> ancestors() const;
    std::string family() const;
    std::string generic() const;
    
    // Compiler support
    std::string optimization_flags(const std::string& compiler, const std::string& version) const;
    
    // Comparison (based on feature hierarchy)
    bool operator<(const Microarchitecture& other) const;
    bool operator>(const Microarchitecture& other) const;
    // ... other comparison operators
};
```

#### `archspec::MicroarchitectureDatabase`

Singleton database of all known microarchitectures.

```cpp
class MicroarchitectureDatabase {
    static MicroarchitectureDatabase& instance();
    
    const Microarchitecture* get(const std::string& name) const;
    bool exists(const std::string& name) const;
    std::vector<std::string> all_names() const;
    const std::map<std::string, Microarchitecture>& all() const;
};
```

### Functions

```cpp
// Detect the host CPU
archspec::Microarchitecture host();

// Get machine architecture string ("x86_64", "aarch64", etc.)
std::string get_machine();

// Get a microarchitecture by name
const Microarchitecture* get_target(const std::string& name);

// Get CPU brand string (if available)
std::optional<std::string> brand_string();

// Create a generic microarchitecture
Microarchitecture generic_microarchitecture(const std::string& name);
```

## Supported Microarchitectures

### x86_64

- Generic: `x86_64`, `x86_64_v2`, `x86_64_v3`, `x86_64_v4`
- Intel: `core2`, `nehalem`, `westmere`, `sandybridge`, `ivybridge`, `haswell`, `broadwell`, `skylake`, `skylake_avx512`, `cascadelake`, `icelake`, `sapphirerapids`
- AMD: `k10`, `bulldozer`, `piledriver`, `steamroller`, `excavator`, `zen`, `zen2`, `zen3`, `zen4`, `zen5`

### AArch64 (ARM64)

- Generic: `aarch64`, `armv8.1a` - `armv8.6a`, `armv9.0a`
- ARM: `cortex_a72`, `neoverse_n1`, `neoverse_v1`, `neoverse_v2`, `neoverse_n2`
- Apple: `m1`, `m2`, `m3`, `m4`
- Others: `thunderx2`, `a64fx`

### PPC64/PPC64LE

- `power7`, `power8`, `power9`, `power10`
- `power8le`, `power9le`, `power10le`

### RISC-V 64

- `riscv64`, `u74mc`

## Integration with Julia/LLVM

This library is designed to be easily integrated with the Julia language and LLVM. The typical usage pattern:

```cpp
#include <archspec/archspec.hpp>

// Detect host for JIT compilation target
archspec::Microarchitecture host = archspec::host();

// Get LLVM-compatible target name
std::string llvm_target = host.name();

// Get compiler flags for the build
std::string clang_flags = host.optimization_flags("clang", "15.0");

// Check for specific vectorization support
bool can_use_avx512 = host.has_feature("avx512f");
bool can_use_sve = host.has_feature("sve");
```

## License

This library is dual-licensed under Apache-2.0 OR MIT, matching the original archspec project.

## Credits

- Based on [archspec](https://github.com/archspec/archspec) by Lawrence Livermore National Security, LLC
- Uses [nlohmann/json](https://github.com/nlohmann/json) for JSON parsing

