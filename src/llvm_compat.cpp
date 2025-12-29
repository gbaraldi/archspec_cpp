// LLVM feature name compatibility layer implementation

#include "archspec/llvm_compat.hpp"
#include "archspec/microarchitecture.hpp"
#include <unordered_map>
#include <cctype>

namespace archspec {

// ============================================================================
// AArch64 feature mapping
// ============================================================================

static const std::unordered_map<std::string, std::string> aarch64_feature_map = {
    {"asimd", "neon"},       {"asimddp", "dotprod"},  {"asimdfhm", "fp16fml"},
    {"asimdhp", "fullfp16"}, {"asimdrdm", "rdm"},     {"atomics", "lse"},
    {"crc32", "crc"},        {"fcma", "complxnum"},   {"fp", "fp-armv8"},
    {"fphp", "fullfp16"},    {"jscvt", "jsconv"},     {"lrcpc", "rcpc"},
    {"ilrcpc", "rcpc-immo"}, {"paca", "pauth"},       {"pacg", "pauth"},
    {"rng", "rand"},
};

// Features to filter out (no LLVM equivalent or implied by other features)
static const std::set<std::string> aarch64_filter_out = {
    "cpuid", "dcpodp", "dcpop", "dgh", "evtstrm", "flagm2", "frint",
    "uscat", "sha1", "sha512", "pmull",
    "svebf16",  // implied by sve + bf16
    "svei8mm",  // implied by sve + i8mm
};

// ============================================================================
// x86_64 feature mapping
// ============================================================================

static const std::unordered_map<std::string, std::string> x86_64_feature_map = {
    {"sse4_1", "sse4.1"},
    {"sse4_2", "sse4.2"},
    {"avx512_vnni", "avx512vnni"},
    {"avx512_bf16", "avx512bf16"},
    {"avx512_vbmi", "avx512vbmi"},
    {"avx512_vbmi2", "avx512vbmi2"},
    {"avx512_ifma", "avx512ifma"},
    {"avx512_vpopcntdq", "avx512vpopcntdq"},
    {"avx512_vp2intersect", "avx512vp2intersect"},
    {"avx512_bitalg", "avx512bitalg"},
    {"avx_vnni", "avxvnni"},
    {"lahf_lm", "sahf"},
    {"pclmulqdq", "pclmul"},
    {"rdrand", "rdrnd"},
    {"abm", "lzcnt"},
    {"bmi1", "bmi"},
    {"sha_ni", "sha"},
    {"amx_bf16", "amx-bf16"},
    {"amx_int8", "amx-int8"},
    {"amx_tile", "amx-tile"},
};

static const std::set<std::string> x86_64_filter_out = {
    "3dnow",
    "3dnowext",
    "avx512er",
    "avx512pf",
};

// ============================================================================
// RISC-V feature mapping
// ============================================================================

static const std::unordered_map<std::string, std::string> riscv_feature_map = {
    // RISC-V extensions often match but may need prefix
    {"m", "m"},
    {"a", "a"},
    {"f", "f"},
    {"d", "d"},
    {"c", "c"},
    {"v", "v"},
    {"zba", "zba"},
    {"zbb", "zbb"},
    {"zbc", "zbc"},
    {"zbs", "zbs"},
    {"zfh", "zfh"},
    {"zfhmin", "zfhmin"},
    {"zvl128b", "zvl128b"},
    {"zvl256b", "zvl256b"},
    {"zvl512b", "zvl512b"},
    {"zvl1024b", "zvl1024b"},
};

static const std::set<std::string> riscv_filter_out = {
    // RISC-V usually doesn't need filtering
};

// ============================================================================
// CPU name mapping
// ============================================================================

static const std::unordered_map<std::string, std::string> aarch64_cpu_map = {
    // Apple Silicon: archspec uses short names, LLVM uses apple- prefix
    {"m1", "apple-m1"},
    {"m1_pro", "apple-m1"},
    {"m1_max", "apple-m1"},
    {"m1_ultra", "apple-m1"},
    {"m2", "apple-m2"},
    {"m2_pro", "apple-m2"},
    {"m2_max", "apple-m2"},
    {"m2_ultra", "apple-m2"},
    {"m3", "apple-m3"},
    {"m3_pro", "apple-m3"},
    {"m3_max", "apple-m3"},
    {"m3_ultra", "apple-m3"},
    {"m4", "apple-m4"},
    {"m4_pro", "apple-m4"},
    {"m4_max", "apple-m4"},
    // A-series
    {"a7", "apple-a7"},
    {"a8", "apple-a8"},
    {"a9", "apple-a9"},
    {"a10", "apple-a10"},
    {"a11", "apple-a11"},
    {"a12", "apple-a12"},
    {"a13", "apple-a13"},
    {"a14", "apple-a14"},
    {"a15", "apple-a15"},
    {"a16", "apple-a16"},
    {"a17", "apple-a17"},
    // Qualcomm
    {"thunderx2", "thunderx2t99"},
    {"thunderx3", "thunderx3t110"},
};

static const std::unordered_map<std::string, std::string> x86_64_cpu_map = {
    // AMD Zen family: archspec uses "zen" names, LLVM uses "znver"
    {"zen", "znver1"},
    {"zen2", "znver2"},
    {"zen3", "znver3"},
    {"zen4", "znver4"},
    // Intel often matches but some differ
    {"icelake", "icelake-client"},
    {"icelake_server", "icelake-server"},
    {"sapphirerapids", "sapphirerapids"},
    {"alderlake", "alderlake"},
    {"meteorlake", "meteorlake"},
};

// ============================================================================
// Implementation
// ============================================================================

std::string map_feature_to_llvm(std::string_view arch_family, std::string_view feature) {
    std::string feat_str(feature);

    if (arch_family == "aarch64") {
        // Check if should be filtered
        if (aarch64_filter_out.count(feat_str)) {
            return "";
        }
        // Check for mapping
        auto it = aarch64_feature_map.find(feat_str);
        if (it != aarch64_feature_map.end()) {
            return it->second;
        }
    } else if (arch_family == "x86_64" || arch_family == "x86") {
        if (x86_64_filter_out.count(feat_str)) {
            return "";
        }
        auto it = x86_64_feature_map.find(feat_str);
        if (it != x86_64_feature_map.end()) {
            return it->second;
        }
    } else if (arch_family == "riscv64" || arch_family == "riscv32") {
        if (riscv_filter_out.count(feat_str)) {
            return "";
        }
        auto it = riscv_feature_map.find(feat_str);
        if (it != riscv_feature_map.end()) {
            return it->second;
        }
    }

    // No mapping needed, return as-is
    return feat_str;
}

std::set<std::string> get_llvm_features(const Microarchitecture& uarch) {
    std::set<std::string> result;
    std::string family = uarch.family();

    for (const auto& feat : uarch.features()) {
        std::string mapped = map_feature_to_llvm(family, feat);
        if (!mapped.empty()) {
            result.insert(mapped);
        }
    }

    return result;
}

std::string get_llvm_features_string(const Microarchitecture& uarch) {
    auto features = get_llvm_features(uarch);
    std::string result;

    for (const auto& feat : features) {
        if (!result.empty()) {
            result += ",";
        }
        result += "+" + feat;
    }

    return result;
}

std::string get_llvm_cpu_name(const Microarchitecture& uarch) {
    std::string name = uarch.name();
    std::string family = uarch.family();
    std::string vendor = uarch.vendor();

    // Check architecture-specific CPU name mappings
    if (family == "aarch64") {
        // Apple Silicon special handling
        if (vendor == "Apple" || vendor == "apple") {
            auto it = aarch64_cpu_map.find(name);
            if (it != aarch64_cpu_map.end()) {
                return it->second;
            }
            // Fallback: if name starts with 'm' or 'a', add apple- prefix
            if (!name.empty() && (name[0] == 'm' || name[0] == 'a')) {
                return "apple-" + name;
            }
        }
        auto it = aarch64_cpu_map.find(name);
        if (it != aarch64_cpu_map.end()) {
            return it->second;
        }
    } else if (family == "x86_64" || family == "x86") {
        auto it = x86_64_cpu_map.find(name);
        if (it != x86_64_cpu_map.end()) {
            return it->second;
        }
    }

    // No mapping needed
    return name;
}

// ============================================================================
// Reverse CPU name mapping (LLVM -> archspec)
// ============================================================================

static const std::unordered_map<std::string, std::string> aarch64_cpu_reverse_map = {
    // Apple Silicon: LLVM uses apple- prefix, archspec uses short names
    {"apple-m1", "m1"},
    {"apple-m2", "m2"},
    {"apple-m3", "m3"},
    {"apple-m4", "m4"},
    {"apple-a7", "a7"},
    {"apple-a8", "a8"},
    {"apple-a9", "a9"},
    {"apple-a10", "a10"},
    {"apple-a11", "a11"},
    {"apple-a12", "a12"},
    {"apple-a13", "a13"},
    {"apple-a14", "a14"},
    {"apple-a15", "a15"},
    {"apple-a16", "a16"},
    {"apple-a17", "a17"},
    // Qualcomm - thunderx2 is in DB
    {"thunderx2t99", "thunderx2"},
    {"thunderx3t110", "thunderx2"},  // thunderx3 not in DB, use thunderx2
    // ARM Cortex - only cortex_a72 is in DB, map older ones to aarch64 base
    {"cortex-a35", "aarch64"},
    {"cortex-a53", "aarch64"},
    {"cortex-a55", "aarch64"},
    {"cortex-a57", "aarch64"},
    {"cortex-a65", "aarch64"},
    {"cortex-a72", "cortex_a72"},
    {"cortex-a73", "cortex_a72"},
    {"cortex-a75", "cortex_a72"},
    {"cortex-a76", "cortex_a72"},
    {"cortex-a77", "cortex_a72"},
    {"cortex-a78", "cortex_a72"},
    {"cortex-a710", "cortex_a72"},
    {"cortex-x1", "cortex_a72"},
    {"cortex-x2", "cortex_a72"},
    {"cortex-x3", "cortex_a72"},
    // Neoverse - all are in DB
    {"neoverse-n1", "neoverse_n1"},
    {"neoverse-n2", "neoverse_n2"},
    {"neoverse-v1", "neoverse_v1"},
    {"neoverse-v2", "neoverse_v2"},
    // Nvidia
    {"carmel", "aarch64"},
    // Ampere
    {"ampere1", "neoverse_n1"},
    {"ampere1a", "neoverse_n1"},
};

static const std::unordered_map<std::string, std::string> x86_64_cpu_reverse_map = {
    // AMD Zen family: LLVM uses "znver", archspec uses "zen"
    {"znver1", "zen"},
    {"znver2", "zen2"},
    {"znver3", "zen3"},
    {"znver4", "zen4"},
    // Intel
    {"icelake-client", "icelake"},
    {"icelake-server", "icelake_server"},
    {"skylake-avx512", "skylake_avx512"},
    {"cascadelake", "cascadelake"},
    {"cooperlake", "cooperlake"},
};

std::string normalize_cpu_name(std::string_view arch_family, std::string_view llvm_name) {
    std::string name_str(llvm_name);

    if (arch_family == "aarch64") {
        auto it = aarch64_cpu_reverse_map.find(name_str);
        if (it != aarch64_cpu_reverse_map.end()) {
            return it->second;
        }
        // Handle apple- prefix generically
        if (name_str.substr(0, 6) == "apple-") {
            return name_str.substr(6);
        }
        // Handle cortex- prefix (replace - with _)
        if (name_str.find("cortex-") == 0 || name_str.find("neoverse-") == 0) {
            std::string result = name_str;
            for (auto& c : result) {
                if (c == '-')
                    c = '_';
            }
            return result;
        }
    } else if (arch_family == "x86_64" || arch_family == "x86") {
        auto it = x86_64_cpu_reverse_map.find(name_str);
        if (it != x86_64_cpu_reverse_map.end()) {
            return it->second;
        }
        // Handle skylake-avx512 style names
        std::string result = name_str;
        for (auto& c : result) {
            if (c == '-')
                c = '_';
        }
        return result;
    }

    // No mapping needed
    return name_str;
}

std::string get_llvm_features_for_cpu(std::string_view cpu_name, std::string_view arch_family) {
    auto& db = MicroarchitectureDatabase::instance();
    std::string name_str(cpu_name);

    // Skip special names
    if (name_str == "native" || name_str == "generic") {
        return "";
    }

    // Try direct lookup first
    auto uarch_ref = db.get(name_str);
    if (uarch_ref) {
        return get_llvm_features_string(uarch_ref->get());
    }

    // Try normalized name
    std::string normalized = normalize_cpu_name(arch_family, cpu_name);
    if (normalized != name_str) {
        uarch_ref = db.get(normalized);
        if (uarch_ref) {
            return get_llvm_features_string(uarch_ref->get());
        }
    }

    // Try lowercase
    std::string lower = name_str;
    for (auto& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (lower != name_str) {
        uarch_ref = db.get(lower);
        if (uarch_ref) {
            return get_llvm_features_string(uarch_ref->get());
        }
    }

    // Not found
    return "";
}

} // namespace archspec
