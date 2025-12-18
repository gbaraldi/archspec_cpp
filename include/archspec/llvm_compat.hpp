// LLVM feature name compatibility layer
// Maps archspec feature names to LLVM feature names

#ifndef ARCHSPEC_LLVM_COMPAT_HPP
#define ARCHSPEC_LLVM_COMPAT_HPP

#include <string>
#include <string_view>
#include <set>
#include <vector>

namespace archspec {

// Forward declaration
class Microarchitecture;

/**
 * Map a single archspec feature name to LLVM feature name.
 * Returns empty string if the feature should be filtered out (no LLVM equivalent).
 * Returns the input unchanged if no mapping is needed.
 *
 * @param arch_family Architecture family ("x86_64", "aarch64", "ppc64le", "riscv64")
 * @param feature The archspec feature name
 * @return LLVM-compatible feature name, or empty string to filter
 */
std::string map_feature_to_llvm(std::string_view arch_family, std::string_view feature);

/**
 * Get all LLVM-compatible features for a microarchitecture.
 * Handles mapping and filtering automatically.
 *
 * @param uarch The microarchitecture to get features for
 * @return Set of LLVM-compatible feature names (without +/- prefix)
 */
std::set<std::string> get_llvm_features(const Microarchitecture& uarch);

/**
 * Get LLVM-compatible features as a comma-separated string with +prefix.
 * Example: "+neon,+dotprod,+crc"
 *
 * @param uarch The microarchitecture to get features for
 * @return Comma-separated feature string
 */
std::string get_llvm_features_string(const Microarchitecture& uarch);

/**
 * Map archspec CPU name to LLVM CPU name.
 * Examples:
 *   - "m4" (Apple) -> "apple-m4"
 *   - "zen3" -> "znver3"
 *
 * @param uarch The microarchitecture
 * @return LLVM-compatible CPU name
 */
std::string get_llvm_cpu_name(const Microarchitecture& uarch);

/**
 * Normalize a CPU name to archspec format (reverse of get_llvm_cpu_name).
 * Examples:
 *   - "apple-m4" -> "m4"
 *   - "znver3" -> "zen3"
 *
 * @param arch_family Architecture family ("x86_64", "aarch64", etc.)
 * @param llvm_name The LLVM CPU name
 * @return archspec-compatible CPU name
 */
std::string normalize_cpu_name(std::string_view arch_family, std::string_view llvm_name);

/**
 * Look up a CPU by name (with normalization) and return its LLVM-compatible features.
 * Tries both the given name and normalized versions.
 *
 * @param cpu_name CPU name (can be LLVM or archspec format)
 * @param arch_family Architecture family for normalization
 * @return LLVM-compatible feature string, or empty if CPU not found
 */
std::string get_llvm_features_for_cpu(std::string_view cpu_name, std::string_view arch_family);

} // namespace archspec

#endif // ARCHSPEC_LLVM_COMPAT_HPP
