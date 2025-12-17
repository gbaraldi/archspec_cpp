// This file is a part of Julia. License is MIT: https://julialang.org/license

#ifndef ARCHSPEC_CPUID_HPP
#define ARCHSPEC_CPUID_HPP

#include <cstdint>
#include <string>
#include <set>

namespace archspec {

/**
 * CPUID register values
 */
struct CpuidRegisters {
    uint32_t eax = 0;
    uint32_t ebx = 0;
    uint32_t ecx = 0;
    uint32_t edx = 0;
};

/**
 * CPUID wrapper class for x86/x86_64 processors
 */
class Cpuid {
  public:
    Cpuid();

    // Execute CPUID instruction with given inputs
    CpuidRegisters query(uint32_t eax, uint32_t ecx = 0) const;

    // Get CPU vendor string (e.g., "GenuineIntel", "AuthenticAMD")
    std::string vendor() const {
        return vendor_;
    }

    // Get highest basic CPUID function supported
    uint32_t highest_basic_function() const {
        return highest_basic_;
    }

    // Get highest extended CPUID function supported
    uint32_t highest_extended_function() const {
        return highest_extended_;
    }

    // Get all detected CPU features
    const std::set<std::string>& features() const {
        return features_;
    }

    // Get CPU brand string (e.g., "Intel(R) Core(TM) i7-...")
    std::string brand_string() const;

    // Check if CPUID is supported
    static bool is_supported();

  private:
    void detect_features();
    bool is_bit_set(uint32_t reg, int bit) const;

    std::string vendor_;
    uint32_t highest_basic_ = 0;
    uint32_t highest_extended_ = 0;
    std::set<std::string> features_;
};

} // namespace archspec

#endif // ARCHSPEC_CPUID_HPP
