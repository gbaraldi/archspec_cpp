// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Example: List all known microarchitectures

#include <archspec/archspec.hpp>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <algorithm>

int main(int argc, char* argv[]) {
    // Optional: filter by family
    std::string filter_family;
    if (argc > 1) {
        filter_family = argv[1];
    }
    
    std::cout << "=== archspec_cpp Known Microarchitectures ===" << std::endl;
    std::cout << std::endl;
    
    const auto& db = archspec::MicroarchitectureDatabase::instance();
    
    // Group targets by family
    std::map<std::string, std::vector<const archspec::Microarchitecture*>> by_family;
    
    for (const auto& [name, target] : db.all()) {
        std::string family = target.family();
        
        if (!filter_family.empty() && family != filter_family) {
            continue;
        }
        
        by_family[family].push_back(&target);
    }
    
    // Print targets grouped by family
    for (auto& [family, targets] : by_family) {
        std::cout << "=== " << family << " ===" << std::endl;
        
        // Sort by ancestry depth
        std::sort(targets.begin(), targets.end(), 
            [](const archspec::Microarchitecture* a, const archspec::Microarchitecture* b) {
                return a->ancestors().size() < b->ancestors().size();
            });
        
        for (const auto* target : targets) {
            std::cout << "  " << std::setw(20) << std::left << target->name();
            std::cout << " vendor=" << std::setw(15) << target->vendor();
            std::cout << " features=" << target->features().size();
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    
    if (filter_family.empty()) {
        std::cout << "Total: " << db.all().size() << " microarchitectures" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Usage: " << argv[0] << " [family]" << std::endl;
    std::cout << "  Family can be: x86_64, aarch64, ppc64le, ppc64, riscv64, etc." << std::endl;
    
    return 0;
}

