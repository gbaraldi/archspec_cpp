// Test LLVM compatibility functions

#include "archspec/archspec.hpp"
#include <cstdio>
#include <cstdlib>
#include <cassert>

void test_aarch64_feature_mapping() {
    printf("Testing AArch64 feature mapping...\n");

    // Test direct mappings
    assert(archspec::map_feature_to_llvm("aarch64", "asimd") == "neon");
    assert(archspec::map_feature_to_llvm("aarch64", "asimddp") == "dotprod");
    assert(archspec::map_feature_to_llvm("aarch64", "crc32") == "crc");
    assert(archspec::map_feature_to_llvm("aarch64", "atomics") == "lse");
    assert(archspec::map_feature_to_llvm("aarch64", "fcma") == "complxnum");
    assert(archspec::map_feature_to_llvm("aarch64", "lrcpc") == "rcpc");
    assert(archspec::map_feature_to_llvm("aarch64", "paca") == "pauth");

    // Test filter-out features
    assert(archspec::map_feature_to_llvm("aarch64", "cpuid") == "");
    assert(archspec::map_feature_to_llvm("aarch64", "evtstrm") == "");
    assert(archspec::map_feature_to_llvm("aarch64", "sha1") == "");

    // Test pass-through features
    assert(archspec::map_feature_to_llvm("aarch64", "aes") == "aes");
    assert(archspec::map_feature_to_llvm("aarch64", "sha2") == "sha2");
    assert(archspec::map_feature_to_llvm("aarch64", "sha3") == "sha3");
    assert(archspec::map_feature_to_llvm("aarch64", "bf16") == "bf16");

    printf("  PASS: AArch64 feature mapping\n");
}

void test_x86_feature_mapping() {
    printf("Testing x86_64 feature mapping...\n");

    // Test mappings
    assert(archspec::map_feature_to_llvm("x86_64", "sse4_1") == "sse4.1");
    assert(archspec::map_feature_to_llvm("x86_64", "sse4_2") == "sse4.2");
    assert(archspec::map_feature_to_llvm("x86_64", "avx512_vnni") == "avx512vnni");

    // Test cpuinfo/archspec -> LLVM name mappings
    assert(archspec::map_feature_to_llvm("x86_64", "lahf_lm") == "sahf");
    assert(archspec::map_feature_to_llvm("x86_64", "pclmulqdq") == "pclmul");
    assert(archspec::map_feature_to_llvm("x86_64", "rdrand") == "rdrnd");
    assert(archspec::map_feature_to_llvm("x86_64", "abm") == "lzcnt");
    assert(archspec::map_feature_to_llvm("x86_64", "bmi1") == "bmi");
    assert(archspec::map_feature_to_llvm("x86_64", "sha_ni") == "sha");
    // AMX features use hyphens in LLVM
    assert(archspec::map_feature_to_llvm("x86_64", "amx_bf16") == "amx-bf16");
    assert(archspec::map_feature_to_llvm("x86_64", "amx_int8") == "amx-int8");
    assert(archspec::map_feature_to_llvm("x86_64", "amx_tile") == "amx-tile");
    // AVX-512 features
    assert(archspec::map_feature_to_llvm("x86_64", "avx512_vp2intersect") == "avx512vp2intersect");

    // Test filter-out (features with no LLVM equivalent)
    assert(archspec::map_feature_to_llvm("x86_64", "3dnow") == "");
    assert(archspec::map_feature_to_llvm("x86_64", "avx512er") == "");
    assert(archspec::map_feature_to_llvm("x86_64", "avx512pf") == "");

    // Test pass-through
    assert(archspec::map_feature_to_llvm("x86_64", "avx") == "avx");
    assert(archspec::map_feature_to_llvm("x86_64", "avx2") == "avx2");
    assert(archspec::map_feature_to_llvm("x86_64", "fma") == "fma");
    assert(archspec::map_feature_to_llvm("x86_64", "bmi2") == "bmi2");
    assert(archspec::map_feature_to_llvm("x86_64", "popcnt") == "popcnt");

    printf("  PASS: x86_64 feature mapping\n");
}

void test_cpu_name_mapping() {
    printf("Testing CPU name mapping...\n");

    // Use singleton database
    auto& db = archspec::MicroarchitectureDatabase::instance();

    // Test AMD Zen mapping
    auto zen3_ref = db.get("zen3");
    if (zen3_ref) {
        std::string llvm_name = archspec::get_llvm_cpu_name(zen3_ref->get());
        printf("  zen3 -> %s\n", llvm_name.c_str());
        assert(llvm_name == "znver3");
    } else {
        printf("  zen3 not in database, skipping\n");
    }

    // Test Apple M-series (if in database)
    auto m1_ref = db.get("m1");
    if (m1_ref) {
        std::string llvm_name = archspec::get_llvm_cpu_name(m1_ref->get());
        printf("  m1 -> %s\n", llvm_name.c_str());
        assert(llvm_name == "apple-m1");
    } else {
        printf("  m1 not in database, skipping\n");
    }

    printf("  PASS: CPU name mapping\n");
}

void test_host_llvm_features() {
    printf("Testing host LLVM features...\n");

    auto host = archspec::host();
    if (!host.valid()) {
        printf("  SKIP: Could not detect host\n");
        return;
    }

    printf("  Host: %s (%s)\n", host.name().c_str(), host.family().c_str());

    std::string llvm_cpu = archspec::get_llvm_cpu_name(host);
    printf("  LLVM CPU name: %s\n", llvm_cpu.c_str());

    auto llvm_features = archspec::get_llvm_features(host);
    printf("  LLVM features (%zu):", llvm_features.size());
    int count = 0;
    for (const auto& f : llvm_features) {
        if (count++ < 10) {
            printf(" %s", f.c_str());
        }
    }
    if (count > 10) {
        printf(" ... (%d more)", count - 10);
    }
    printf("\n");

    std::string features_str = archspec::get_llvm_features_string(host);
    printf("  Feature string (first 100 chars): %.100s%s\n", features_str.c_str(),
           features_str.size() > 100 ? "..." : "");

    printf("  PASS: host LLVM features\n");
}

void test_cpu_name_normalization() {
    printf("Testing CPU name normalization...\n");

    // Test AArch64 reverse mappings
    assert(archspec::normalize_cpu_name("aarch64", "apple-m4") == "m4");
    assert(archspec::normalize_cpu_name("aarch64", "apple-m1") == "m1");
    assert(archspec::normalize_cpu_name("aarch64", "apple-a15") == "a15");
    assert(archspec::normalize_cpu_name("aarch64", "cortex-a72") == "cortex_a72");
    assert(archspec::normalize_cpu_name("aarch64", "neoverse-n1") == "neoverse_n1");

    // Test x86 reverse mappings
    assert(archspec::normalize_cpu_name("x86_64", "znver3") == "zen3");
    assert(archspec::normalize_cpu_name("x86_64", "znver4") == "zen4");
    assert(archspec::normalize_cpu_name("x86_64", "icelake-client") == "icelake");

    // Test pass-through
    assert(archspec::normalize_cpu_name("x86_64", "haswell") == "haswell");
    assert(archspec::normalize_cpu_name("aarch64", "generic") == "generic");

    printf("  PASS: CPU name normalization\n");
}

void test_features_for_cpu() {
    printf("Testing features lookup by CPU name...\n");

    // Test AArch64 with LLVM name
    std::string m1_features = archspec::get_llvm_features_for_cpu("apple-m1", "aarch64");
    printf("  apple-m1 features: %.60s%s\n", m1_features.c_str(),
           m1_features.size() > 60 ? "..." : "");
    assert(!m1_features.empty());
    assert(m1_features.find("+neon") != std::string::npos ||
           m1_features.find("neon") != std::string::npos);

    // Test x86 with LLVM name
    std::string zen3_features = archspec::get_llvm_features_for_cpu("znver3", "x86_64");
    printf("  znver3 features: %.60s%s\n", zen3_features.c_str(),
           zen3_features.size() > 60 ? "..." : "");
    assert(!zen3_features.empty());

    // Test with archspec name directly
    std::string haswell_features = archspec::get_llvm_features_for_cpu("haswell", "x86_64");
    printf("  haswell features: %.60s%s\n", haswell_features.c_str(),
           haswell_features.size() > 60 ? "..." : "");
    assert(!haswell_features.empty());
    assert(haswell_features.find("+avx2") != std::string::npos ||
           haswell_features.find("avx2") != std::string::npos);

    // Test generic returns empty
    std::string generic_features = archspec::get_llvm_features_for_cpu("generic", "x86_64");
    assert(generic_features.empty());

    printf("  PASS: features lookup by CPU name\n");
}

int main() {
    printf("=== LLVM Compatibility Tests ===\n\n");

    test_aarch64_feature_mapping();
    test_x86_feature_mapping();
    test_cpu_name_mapping();
    test_host_llvm_features();
    test_cpu_name_normalization();
    test_features_for_cpu();

    printf("\n=== All LLVM compatibility tests passed! ===\n");
    return 0;
}
