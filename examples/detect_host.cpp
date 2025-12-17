// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Example: Detect the host CPU microarchitecture

#include <archspec/archspec.hpp>
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "=== archspec_cpp Host Detection ===" << std::endl;
    std::cout << std::endl;
    
    // Get machine architecture
    std::string machine = archspec::get_machine();
    std::cout << "Machine architecture: " << machine << std::endl;
    
    // Get brand string if available
    auto brand = archspec::brand_string();
    if (brand.has_value()) {
        std::cout << "CPU brand string: " << *brand << std::endl;
    }
    std::cout << std::endl;
    
    // Detect the host microarchitecture
    archspec::Microarchitecture host = archspec::host();
    
    std::cout << "Detected microarchitecture:" << std::endl;
    std::cout << "  Name: " << host.name() << std::endl;
    std::cout << "  Vendor: " << host.vendor() << std::endl;
    std::cout << "  Family: " << host.family() << std::endl;
    std::cout << "  Generic: " << host.generic() << std::endl;
    
    // Show ancestors
    auto ancestors = host.ancestors();
    if (!ancestors.empty()) {
        std::cout << "  Ancestors: ";
        for (size_t i = 0; i < ancestors.size(); ++i) {
            if (i > 0) std::cout << " -> ";
            std::cout << ancestors[i];
        }
        std::cout << std::endl;
    }
    
    // Show features
    const auto& features = host.features();
    if (!features.empty()) {
        std::cout << "  Features (" << features.size() << "):" << std::endl;
        std::cout << "    ";
        int col = 4;
        for (const auto& f : features) {
            if (col + f.length() + 1 > 80) {
                std::cout << std::endl << "    ";
                col = 4;
            }
            std::cout << f << " ";
            col += f.length() + 1;
        }
        std::cout << std::endl;
    }
    
    std::cout << std::endl;
    
    // Show compiler flags
    std::cout << "Compiler optimization flags:" << std::endl;
    
    std::string gcc_flags = host.optimization_flags("gcc", "10.0");
    if (!gcc_flags.empty()) {
        std::cout << "  GCC 10.0: " << gcc_flags << std::endl;
    }
    
    std::string clang_flags = host.optimization_flags("clang", "12.0");
    if (!clang_flags.empty()) {
        std::cout << "  Clang 12.0: " << clang_flags << std::endl;
    }
    
    std::cout << std::endl;
    
    // Show LLVM target info (useful for Julia integration)
    std::cout << "For LLVM/Julia integration:" << std::endl;
    std::cout << "  Target: " << host.name() << std::endl;
    std::cout << "  Family: " << host.family() << std::endl;
    
    // Show some specific feature checks
    std::cout << std::endl << "Feature checks:" << std::endl;
    if (host.family() == "x86_64") {
        std::cout << "  Has AVX: " << (host.has_feature("avx") ? "yes" : "no") << std::endl;
        std::cout << "  Has AVX2: " << (host.has_feature("avx2") ? "yes" : "no") << std::endl;
        std::cout << "  Has AVX-512: " << (host.has_feature("avx512f") ? "yes" : "no") << std::endl;
        std::cout << "  Has FMA: " << (host.has_feature("fma") ? "yes" : "no") << std::endl;
    } else if (host.family() == "aarch64") {
        std::cout << "  Has NEON: " << (host.has_feature("neon") ? "yes" : "no") << std::endl;
        std::cout << "  Has SVE: " << (host.has_feature("sve") ? "yes" : "no") << std::endl;
    }
    
    return 0;
}

