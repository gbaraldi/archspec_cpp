// This file is a part of Julia. License is MIT: https://julialang.org/license

#include "archspec/cpuid.hpp"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#define ARCHSPEC_X86 1
#endif

#ifdef ARCHSPEC_X86

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#endif

#include <cstring>

namespace archspec {

bool Cpuid::is_supported() {
#ifdef ARCHSPEC_X86
    return true;
#else
    return false;
#endif
}

CpuidRegisters Cpuid::query(uint32_t eax_in, uint32_t ecx_in) const {
    CpuidRegisters regs;

#if defined(_MSC_VER)
    int cpu_info[4];
    __cpuidex(cpu_info, static_cast<int>(eax_in), static_cast<int>(ecx_in));
    regs.eax = static_cast<uint32_t>(cpu_info[0]);
    regs.ebx = static_cast<uint32_t>(cpu_info[1]);
    regs.ecx = static_cast<uint32_t>(cpu_info[2]);
    regs.edx = static_cast<uint32_t>(cpu_info[3]);
#elif defined(__GNUC__) || defined(__clang__)
    __cpuid_count(eax_in, ecx_in, regs.eax, regs.ebx, regs.ecx, regs.edx);
#endif

    return regs;
}

Cpuid::Cpuid() {
    // Get vendor string and highest basic function
    CpuidRegisters regs = query(0, 0);
    highest_basic_ = regs.eax;

    // Vendor string is in EBX, EDX, ECX (in that order)
    char vendor_buf[13] = {0};
    std::memcpy(vendor_buf, &regs.ebx, 4);
    std::memcpy(vendor_buf + 4, &regs.edx, 4);
    std::memcpy(vendor_buf + 8, &regs.ecx, 4);
    vendor_ = vendor_buf;

    // Get highest extended function
    regs = query(0x80000000, 0);
    highest_extended_ = regs.eax;

    // Detect features
    detect_features();
}

bool Cpuid::is_bit_set(uint32_t reg, int bit) const {
    return (reg & (1u << bit)) != 0;
}

void Cpuid::detect_features() {
    // EAX=1: Basic feature flags
    if (highest_basic_ >= 1) {
        CpuidRegisters regs = query(1, 0);

        // EDX features
        if (is_bit_set(regs.edx, 0))
            features_.insert("fpu");
        if (is_bit_set(regs.edx, 23))
            features_.insert("mmx");
        if (is_bit_set(regs.edx, 25))
            features_.insert("sse");
        if (is_bit_set(regs.edx, 26))
            features_.insert("sse2");
        if (is_bit_set(regs.edx, 28))
            features_.insert("ht");

        // ECX features
        if (is_bit_set(regs.ecx, 0))
            features_.insert("pni"); // SSE3
        if (is_bit_set(regs.ecx, 1))
            features_.insert("pclmulqdq");
        if (is_bit_set(regs.ecx, 9))
            features_.insert("ssse3");
        if (is_bit_set(regs.ecx, 12))
            features_.insert("fma");
        if (is_bit_set(regs.ecx, 13))
            features_.insert("cx16");
        if (is_bit_set(regs.ecx, 19))
            features_.insert("sse4_1");
        if (is_bit_set(regs.ecx, 20))
            features_.insert("sse4_2");
        if (is_bit_set(regs.ecx, 22))
            features_.insert("movbe");
        if (is_bit_set(regs.ecx, 23))
            features_.insert("popcnt");
        if (is_bit_set(regs.ecx, 25))
            features_.insert("aes");
        if (is_bit_set(regs.ecx, 26))
            features_.insert("xsave");
        if (is_bit_set(regs.ecx, 28))
            features_.insert("avx");
        if (is_bit_set(regs.ecx, 29))
            features_.insert("f16c");
        if (is_bit_set(regs.ecx, 30))
            features_.insert("rdrand");
    }

    // EAX=7, ECX=0: Extended features
    if (highest_basic_ >= 7) {
        CpuidRegisters regs = query(7, 0);

        // EBX features
        if (is_bit_set(regs.ebx, 0))
            features_.insert("fsgsbase");
        if (is_bit_set(regs.ebx, 3))
            features_.insert("bmi1");
        if (is_bit_set(regs.ebx, 5))
            features_.insert("avx2");
        if (is_bit_set(regs.ebx, 8))
            features_.insert("bmi2");
        if (is_bit_set(regs.ebx, 16))
            features_.insert("avx512f");
        if (is_bit_set(regs.ebx, 17))
            features_.insert("avx512dq");
        if (is_bit_set(regs.ebx, 18))
            features_.insert("rdseed");
        if (is_bit_set(regs.ebx, 19))
            features_.insert("adx");
        if (is_bit_set(regs.ebx, 21))
            features_.insert("avx512ifma");
        if (is_bit_set(regs.ebx, 23))
            features_.insert("clflushopt");
        if (is_bit_set(regs.ebx, 24))
            features_.insert("clwb");
        if (is_bit_set(regs.ebx, 26))
            features_.insert("avx512pf");
        if (is_bit_set(regs.ebx, 27))
            features_.insert("avx512er");
        if (is_bit_set(regs.ebx, 28))
            features_.insert("avx512cd");
        if (is_bit_set(regs.ebx, 29))
            features_.insert("sha_ni");
        if (is_bit_set(regs.ebx, 30))
            features_.insert("avx512bw");
        if (is_bit_set(regs.ebx, 31))
            features_.insert("avx512vl");

        // ECX features
        if (is_bit_set(regs.ecx, 1))
            features_.insert("avx512vbmi");
        if (is_bit_set(regs.ecx, 3))
            features_.insert("pku");
        if (is_bit_set(regs.ecx, 5))
            features_.insert("waitpkg");
        if (is_bit_set(regs.ecx, 6))
            features_.insert("avx512_vbmi2");
        if (is_bit_set(regs.ecx, 8))
            features_.insert("gfni");
        if (is_bit_set(regs.ecx, 9))
            features_.insert("vaes");
        if (is_bit_set(regs.ecx, 10))
            features_.insert("vpclmulqdq");
        if (is_bit_set(regs.ecx, 11))
            features_.insert("avx512_vnni");
        if (is_bit_set(regs.ecx, 12))
            features_.insert("avx512_bitalg");
        if (is_bit_set(regs.ecx, 14))
            features_.insert("avx512_vpopcntdq");
        if (is_bit_set(regs.ecx, 22))
            features_.insert("rdpid");
        if (is_bit_set(regs.ecx, 25))
            features_.insert("cldemote");
        if (is_bit_set(regs.ecx, 27))
            features_.insert("movdiri");
        if (is_bit_set(regs.ecx, 28))
            features_.insert("movdir64b");

        // EDX features
        if (is_bit_set(regs.edx, 8))
            features_.insert("avx512_vp2intersect");
        if (is_bit_set(regs.edx, 14))
            features_.insert("serialize");
        if (is_bit_set(regs.edx, 22))
            features_.insert("amx_bf16");
        if (is_bit_set(regs.edx, 24))
            features_.insert("amx_tile");
        if (is_bit_set(regs.edx, 25))
            features_.insert("amx_int8");
    }

    // EAX=7, ECX=1: More extended features
    if (highest_basic_ >= 7) {
        CpuidRegisters regs = query(7, 1);

        if (is_bit_set(regs.eax, 4))
            features_.insert("avx_vnni");
        if (is_bit_set(regs.eax, 5))
            features_.insert("avx512_bf16");
    }

    // EAX=0xD, ECX=1: XSAVE features
    if (highest_basic_ >= 0xD) {
        CpuidRegisters regs = query(0xD, 1);

        if (is_bit_set(regs.eax, 0))
            features_.insert("xsaveopt");
        if (is_bit_set(regs.eax, 1))
            features_.insert("xsavec");
    }

    // Extended features (0x80000001)
    if (highest_extended_ >= 0x80000001) {
        CpuidRegisters regs = query(0x80000001, 0);

        // ECX features
        if (is_bit_set(regs.ecx, 0))
            features_.insert("lahf_lm");
        if (is_bit_set(regs.ecx, 5))
            features_.insert("abm"); // lzcnt, popcnt
        if (is_bit_set(regs.ecx, 6))
            features_.insert("sse4a");
        if (is_bit_set(regs.ecx, 11))
            features_.insert("xop");
        if (is_bit_set(regs.ecx, 16))
            features_.insert("fma4");
        if (is_bit_set(regs.ecx, 21))
            features_.insert("tbm");

        // EDX features
        if (is_bit_set(regs.edx, 30))
            features_.insert("3dnowext");
        if (is_bit_set(regs.edx, 31))
            features_.insert("3dnow");
    }
}

std::string Cpuid::brand_string() const {
    if (highest_extended_ < 0x80000004) {
        return "";
    }

    char brand[49] = {0};

    CpuidRegisters regs;
    for (uint32_t i = 0; i < 3; ++i) {
        regs = query(0x80000002 + i, 0);
        std::memcpy(brand + i * 16, &regs.eax, 4);
        std::memcpy(brand + i * 16 + 4, &regs.ebx, 4);
        std::memcpy(brand + i * 16 + 8, &regs.ecx, 4);
        std::memcpy(brand + i * 16 + 12, &regs.edx, 4);
    }

    // Trim trailing whitespace
    std::string result(brand);
    while (!result.empty() && (result.back() == ' ' || result.back() == '\0')) {
        result.pop_back();
    }

    return result;
}

} // namespace archspec

#else // !ARCHSPEC_X86

namespace archspec {

bool Cpuid::is_supported() {
    return false;
}

Cpuid::Cpuid() {}

CpuidRegisters Cpuid::query(uint32_t, uint32_t) const {
    return CpuidRegisters{};
}

std::string Cpuid::brand_string() const {
    return "";
}

void Cpuid::detect_features() {}

bool Cpuid::is_bit_set(uint32_t, int) const {
    return false;
}

} // namespace archspec

#endif // ARCHSPEC_X86
