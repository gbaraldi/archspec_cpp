// This file is a part of Julia. License is MIT: https://julialang.org/license

#include "archspec/archspec_c.h"
#include "archspec/archspec.hpp"
#include <cstring>
#include <string>
#include <vector>

// Helper to convert C++ string to allocated C string
static char* to_c_string(const std::string& str) {
    char* result = static_cast<char*>(malloc(str.size() + 1));
    if (result) {
        memcpy(result, str.c_str(), str.size() + 1);
    }
    return result;
}

// Helper to join strings with comma
static std::string join_features(const std::set<std::string>& features) {
    std::string result;
    bool first = true;
    for (const auto& f : features) {
        if (!first) result += ",";
        result += f;
        first = false;
    }
    return result;
}

// Static storage for host name and vendor (avoid repeated allocation)
static std::string s_host_name;
static std::string s_host_vendor;
static std::vector<std::string> s_target_names;
static bool s_initialized = false;

static void ensure_initialized() {
    if (!s_initialized) {
        try {
            auto host_arch = archspec::host();
            s_host_name = host_arch.name();
            
            auto info = archspec::detect_cpu_info();
            s_host_vendor = info.vendor;
            
            // Cache target names
            const auto& db = archspec::MicroarchitectureDatabase::instance();
            s_target_names = db.all_names();
        } catch (...) {
            // Ignore errors during initialization
        }
        s_initialized = true;
    }
}

extern "C" {

const char* archspec_host_name(void) {
    ensure_initialized();
    return s_host_name.empty() ? nullptr : s_host_name.c_str();
}

char* archspec_host_features(void) {
    try {
        auto host_arch = archspec::host();
        return to_c_string(join_features(host_arch.features()));
    } catch (...) {
        return nullptr;
    }
}

const char* archspec_host_vendor(void) {
    ensure_initialized();
    return s_host_vendor.empty() ? nullptr : s_host_vendor.c_str();
}

char* archspec_get_features(const char* name) {
    if (!name) return nullptr;
    try {
        const auto& db = archspec::MicroarchitectureDatabase::instance();
        const auto* target = db.get(name);
        if (!target) return nullptr;
        return to_c_string(join_features(target->features()));
    } catch (...) {
        return nullptr;
    }
}

char* archspec_get_flags(const char* name, const char* compiler) {
    if (!name || !compiler) return nullptr;
    try {
        const auto& db = archspec::MicroarchitectureDatabase::instance();
        const auto* target = db.get(name);
        if (!target) return nullptr;
        // Use empty version string to get default flags
        std::string flags = target->optimization_flags(compiler, "");
        if (flags.empty()) return nullptr;
        return to_c_string(flags);
    } catch (...) {
        return nullptr;
    }
}

char* archspec_host_flags(const char* compiler) {
    if (!compiler) return nullptr;
    try {
        auto host_arch = archspec::host();
        // Use empty version string to get default flags
        std::string flags = host_arch.optimization_flags(compiler, "");
        if (flags.empty()) return nullptr;
        return to_c_string(flags);
    } catch (...) {
        return nullptr;
    }
}

int archspec_has_feature(const char* name, const char* feature) {
    if (!name || !feature) return 0;
    try {
        const auto& db = archspec::MicroarchitectureDatabase::instance();
        const auto* target = db.get(name);
        if (!target) return 0;
        return target->has_feature(feature) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int archspec_host_has_feature(const char* feature) {
    if (!feature) return 0;
    try {
        auto host_arch = archspec::host();
        return host_arch.has_feature(feature) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

size_t archspec_target_count(void) {
    ensure_initialized();
    return s_target_names.size();
}

const char* archspec_target_name(size_t index) {
    ensure_initialized();
    if (index >= s_target_names.size()) return nullptr;
    return s_target_names[index].c_str();
}

int archspec_target_exists(const char* name) {
    if (!name) return 0;
    try {
        const auto& db = archspec::MicroarchitectureDatabase::instance();
        return db.exists(name) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

void archspec_free(char* str) {
    free(str);
}

} // extern "C"

