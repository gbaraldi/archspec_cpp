// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Example: Check for specific CPU features on the host

#include <archspec/archspec.hpp>
#include <iostream>
#include <iomanip>
#include <vector>

int main(int argc, char* argv[]) {
    std::cout << "=== archspec_cpp Feature Check ===" << std::endl;
    std::cout << std::endl;
    
    // Detect host
    archspec::Microarchitecture host = archspec::host();
    std::cout << "Host: " << host.name() << " (" << host.family() << ")" << std::endl;
    std::cout << std::endl;
    
    // Features to check
    std::vector<std::string> features;
    
    if (argc > 1) {
        // Use command line arguments
        for (int i = 1; i < argc; ++i) {
            features.push_back(argv[i]);
        }
    } else {
        // Default features based on architecture
        if (host.family() == "x86_64") {
            features = {
                "sse", "sse2", "sse3", "ssse3", "sse4_1", "sse4_2",
                "avx", "avx2", "avx512f", "avx512vl", "avx512bw",
                "fma", "bmi1", "bmi2", "popcnt", "aes",
                "xsave", "xsavec", "xsaveopt"
            };
        } else if (host.family() == "aarch64") {
            features = {
                "neon", "fp", "asimd", "aes", "sha1", "sha2",
                "crc32", "atomics", "sve", "sve2"
            };
        } else if (host.family() == "ppc64le" || host.family() == "ppc64") {
            features = {
                "altivec", "vsx", "fma"
            };
        } else {
            features = {"fpu", "simd"};  // Generic
        }
    }
    
    // Check each feature
    std::cout << "Feature support:" << std::endl;
    for (const auto& feature : features) {
        bool has = host.has_feature(feature);
        std::cout << "  " << std::setw(20) << std::left << feature;
        std::cout << (has ? "YES" : "no") << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Usage: " << argv[0] << " [feature1] [feature2] ..." << std::endl;
    
    return 0;
}

