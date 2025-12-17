// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Example: Get compiler optimization flags for various targets

#include <archspec/archspec.hpp>
#include <iostream>
#include <iomanip>
#include <vector>

int main(int argc, char* argv[]) {
    std::cout << "=== archspec_cpp Compiler Flags ===" << std::endl;
    std::cout << std::endl;

    // Targets to show flags for
    std::vector<std::string> targets;

    if (argc > 1) {
        // Use command line arguments
        for (int i = 1; i < argc; ++i) {
            targets.push_back(argv[i]);
        }
    } else {
        // Default targets
        targets = {"x86_64",  "x86_64_v2",      "x86_64_v3",   "x86_64_v4", "haswell",
                   "skylake", "skylake_avx512", "zen2",        "zen3",      "zen4",
                   "aarch64", "neoverse_n1",    "neoverse_v2", "m1",        "m2"};
    }

    // Compilers and versions to test
    std::vector<std::pair<std::string, std::string>> compilers = {
        {"gcc", "10.0"},
        {"gcc", "12.0"},
        {"clang", "12.0"},
        {"clang", "15.0"},
    };

    for (const auto& target_name : targets) {
        auto target = archspec::get_target(target_name);

        if (!target) {
            std::cout << target_name << ": NOT FOUND" << std::endl;
            continue;
        }

        std::cout << "=== " << target_name << " ===" << std::endl;
        std::cout << "  Vendor: " << target->get().vendor() << std::endl;
        std::cout << "  Family: " << target->get().family() << std::endl;
        std::cout << std::endl;

        for (const auto& [compiler, version] : compilers) {
            std::string flags = target->get().optimization_flags(compiler, version);

            std::cout << "  " << std::setw(12) << std::left << (compiler + " " + version + ":");
            if (flags.empty()) {
                std::cout << "(not supported)";
            } else {
                std::cout << flags;
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "Usage: " << argv[0] << " [target1] [target2] ..." << std::endl;

    return 0;
}
