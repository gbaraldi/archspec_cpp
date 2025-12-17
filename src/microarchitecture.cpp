// This file is a part of Julia. License is MIT: https://julialang.org/license

#include "archspec/microarchitecture.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <stdexcept>

// Embedded JSON data
#include "microarchitectures_data.inc"

namespace archspec {

Microarchitecture::Microarchitecture(
    const std::string& name, const std::vector<std::string>& parents, const std::string& vendor,
    const std::set<std::string>& features,
    const std::map<std::string, std::vector<CompilerEntry>>& compilers, int generation,
    const std::string& cpu_part)
    : name_(name),
      parent_names_(parents),
      vendor_(vendor),
      features_(features),
      compilers_(compilers),
      generation_(generation),
      cpu_part_(cpu_part) {
    // ssse3 implies sse3; add it if not present
    if (features_.count("ssse3") && !features_.count("sse3")) {
        features_.insert("sse3");
    }
}

bool Microarchitecture::has_feature(const std::string& feature) const {
    // Check direct features first
    if (features_.count(feature)) {
        return true;
    }

    // Check feature aliases
    const auto& db = MicroarchitectureDatabase::instance();
    const auto& aliases = db.feature_aliases();
    auto it = aliases.find(feature);
    if (it != aliases.end()) {
        for (const auto& aliased_feature : it->second) {
            if (features_.count(aliased_feature)) {
                return true;
            }
        }
    }

    // Check family features
    const auto& family_feats = db.family_features();
    auto fit = family_feats.find(feature);
    if (fit != family_feats.end()) {
        std::string fam = family();
        if (fit->second.count(fam)) {
            return true;
        }
    }

    return false;
}

std::vector<std::string> Microarchitecture::ancestors() const {
    std::vector<std::string> result;
    const auto& db = MicroarchitectureDatabase::instance();

    for (const auto& parent_name : parent_names_) {
        result.push_back(parent_name);
        const auto* parent = db.get(parent_name);
        if (parent) {
            auto parent_ancestors = parent->ancestors();
            for (const auto& ancestor : parent_ancestors) {
                if (std::find(result.begin(), result.end(), ancestor) == result.end()) {
                    result.push_back(ancestor);
                }
            }
        }
    }

    return result;
}

std::string Microarchitecture::family() const {
    const auto& db = MicroarchitectureDatabase::instance();
    std::vector<std::string> roots;

    // Check if this target has no parents
    if (parent_names_.empty()) {
        return name_;
    }

    // Find all ancestors with no parents (roots)
    auto all_ancestors = ancestors();
    for (const auto& ancestor_name : all_ancestors) {
        const auto* ancestor = db.get(ancestor_name);
        if (ancestor && ancestor->parent_names().empty()) {
            if (std::find(roots.begin(), roots.end(), ancestor_name) == roots.end()) {
                roots.push_back(ancestor_name);
            }
        }
    }

    // Should have exactly one root
    if (roots.size() == 1) {
        return roots[0];
    }

    // Multiple roots - return first one (shouldn't happen normally)
    return roots.empty() ? name_ : roots[0];
}

std::string Microarchitecture::generic() const {
    const auto& db = MicroarchitectureDatabase::instance();

    // Check if this is generic
    if (vendor_ == "generic") {
        return name_;
    }

    // Find best generic ancestor
    std::string best_generic;
    size_t best_depth = 0;

    auto all_ancestors = ancestors();
    for (const auto& ancestor_name : all_ancestors) {
        const auto* ancestor = db.get(ancestor_name);
        if (ancestor && ancestor->vendor() == "generic") {
            size_t depth = ancestor->ancestors().size();
            if (best_generic.empty() || depth > best_depth) {
                best_generic = ancestor_name;
                best_depth = depth;
            }
        }
    }

    return best_generic.empty() ? family() : best_generic;
}

std::set<std::string> Microarchitecture::to_set() const {
    std::set<std::string> result;
    result.insert(name_);
    for (const auto& ancestor : ancestors()) {
        result.insert(ancestor);
    }
    return result;
}

bool Microarchitecture::operator==(const Microarchitecture& other) const {
    return name_ == other.name_ && vendor_ == other.vendor_ && features_ == other.features_ &&
           parent_names_ == other.parent_names_ && generation_ == other.generation_ &&
           cpu_part_ == other.cpu_part_;
}

bool Microarchitecture::operator!=(const Microarchitecture& other) const {
    return !(*this == other);
}

bool Microarchitecture::operator<(const Microarchitecture& other) const {
    auto this_set = to_set();
    auto other_set = other.to_set();

    // this < other means this_set is a proper subset of other_set
    if (this_set.size() >= other_set.size()) {
        return false;
    }

    for (const auto& elem : this_set) {
        if (other_set.count(elem) == 0) {
            return false;
        }
    }

    return true;
}

bool Microarchitecture::operator<=(const Microarchitecture& other) const {
    return (*this == other) || (*this < other);
}

bool Microarchitecture::operator>(const Microarchitecture& other) const {
    return other < *this;
}

bool Microarchitecture::operator>=(const Microarchitecture& other) const {
    return other <= *this;
}

namespace {

// Helper to parse version string like "4.9" into tuple
std::vector<int> parse_version(const std::string& version) {
    std::vector<int> result;
    std::stringstream ss(version);
    std::string part;
    while (std::getline(ss, part, '.')) {
        if (!part.empty()) {
            try {
                result.push_back(std::stoi(part));
            } catch (...) {
                // Ignore non-numeric parts
            }
        }
    }
    return result;
}

// Compare versions: returns -1 if a < b, 0 if equal, 1 if a > b
int compare_versions(const std::vector<int>& a, const std::vector<int>& b) {
    size_t max_len = std::max(a.size(), b.size());
    for (size_t i = 0; i < max_len; ++i) {
        int va = (i < a.size()) ? a[i] : 0;
        int vb = (i < b.size()) ? b[i] : 0;
        if (va < vb)
            return -1;
        if (va > vb)
            return 1;
    }
    return 0;
}

// Check if version satisfies a version constraint like "4.9:" or "3.9:11.1"
bool satisfies_version(const std::string& constraint, const std::string& version) {
    size_t colon_pos = constraint.find(':');
    if (colon_pos == std::string::npos) {
        // No colon - exact match? (shouldn't happen in practice)
        return constraint == version;
    }

    std::string min_ver = constraint.substr(0, colon_pos);
    std::string max_ver = constraint.substr(colon_pos + 1);

    auto ver = parse_version(version);

    if (!min_ver.empty()) {
        auto min = parse_version(min_ver);
        if (compare_versions(ver, min) < 0) {
            return false;
        }
    }

    if (!max_ver.empty()) {
        auto max = parse_version(max_ver);
        if (compare_versions(ver, max) > 0) {
            return false;
        }
    }

    return true;
}

} // anonymous namespace

std::string Microarchitecture::optimization_flags(const std::string& compiler,
                                                  const std::string& version) const {
    const auto& db = MicroarchitectureDatabase::instance();

    // Check if we have info for this compiler
    auto it = compilers_.find(compiler);
    if (it != compilers_.end() && !it->second.empty()) {
        const auto& entries = it->second;

        for (const auto& entry : entries) {
            if (satisfies_version(entry.versions, version)) {
                // Format the flags
                std::string flags = entry.flags;
                std::string target_name = entry.name.empty() ? name_ : entry.name;

                // Replace {name} placeholder
                size_t pos = 0;
                while ((pos = flags.find("{name}", pos)) != std::string::npos) {
                    flags.replace(pos, 6, target_name);
                    pos += target_name.length();
                }

                return flags;
            }
        }
    }

    // Version not supported or no compiler info - try ancestors
    for (const auto& ancestor_name : ancestors()) {
        const auto* ancestor = db.get(ancestor_name);
        if (ancestor) {
            std::string result = ancestor->optimization_flags(compiler, version);
            if (!result.empty()) {
                return result;
            }
        }
    }

    return "";
}

Microarchitecture generic_microarchitecture(const std::string& name) {
    return Microarchitecture(name, {}, "generic", {}, {}, 0, "");
}

// MicroarchitectureDatabase implementation

MicroarchitectureDatabase& MicroarchitectureDatabase::instance() {
    static MicroarchitectureDatabase db;
    return db;
}

MicroarchitectureDatabase::MicroarchitectureDatabase() {
    load_embedded_data();
}

const Microarchitecture* MicroarchitectureDatabase::get(const std::string& name) const {
    auto it = targets_.find(name);
    return (it != targets_.end()) ? &it->second : nullptr;
}

bool MicroarchitectureDatabase::exists(const std::string& name) const {
    return targets_.count(name) > 0;
}

std::vector<std::string> MicroarchitectureDatabase::all_names() const {
    std::vector<std::string> names;
    names.reserve(targets_.size());
    for (const auto& [name, _] : targets_) {
        names.push_back(name);
    }
    return names;
}

bool MicroarchitectureDatabase::load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_string(buffer.str());
}

bool MicroarchitectureDatabase::load_from_json_internal(const void* json_ptr) {
    try {
        const nlohmann::json& j = *static_cast<const nlohmann::json*>(json_ptr);

        // Parse microarchitectures
        if (j.contains("microarchitectures")) {
            const auto& uarchs = j["microarchitectures"];
            for (auto it = uarchs.begin(); it != uarchs.end(); ++it) {
                fill_target(it.key(), &it.value());
            }
        }

        // Parse feature aliases
        if (j.contains("feature_aliases")) {
            const auto& aliases = j["feature_aliases"];
            for (auto it = aliases.begin(); it != aliases.end(); ++it) {
                const auto& alias_data = it.value();
                std::set<std::string> features;

                if (alias_data.contains("any_of")) {
                    for (const auto& f : alias_data["any_of"]) {
                        features.insert(f.get<std::string>());
                    }
                    feature_aliases_[it.key()] = features;
                }

                if (alias_data.contains("families")) {
                    std::set<std::string> families;
                    for (const auto& f : alias_data["families"]) {
                        families.insert(f.get<std::string>());
                    }
                    family_features_[it.key()] = families;
                }
            }
        }

        // Parse conversions
        if (j.contains("conversions")) {
            const auto& conv = j["conversions"];

            if (conv.contains("darwin_flags")) {
                for (auto it = conv["darwin_flags"].begin(); it != conv["darwin_flags"].end();
                     ++it) {
                    darwin_flags_[it.key()] = it.value().get<std::string>();
                }
            }

            if (conv.contains("arm_vendors")) {
                for (auto it = conv["arm_vendors"].begin(); it != conv["arm_vendors"].end(); ++it) {
                    arm_vendors_[it.key()] = it.value().get<std::string>();
                }
            }
        }

        loaded_ = true;
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool MicroarchitectureDatabase::load_from_string(const std::string& json_data) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_data);
        return load_from_json_internal(&j);
    } catch (const std::exception& e) {
        return false;
    }
}

void MicroarchitectureDatabase::fill_target(const std::string& name, const void* json_ptr) {
    // Check if already filled
    if (targets_.count(name)) {
        return;
    }

    const nlohmann::json& data = *static_cast<const nlohmann::json*>(json_ptr);

    // First, recursively fill parents
    std::vector<std::string> parents;
    if (data.contains("from")) {
        for (const auto& parent : data["from"]) {
            std::string parent_name = parent.get<std::string>();
            parents.push_back(parent_name);
        }
    }

    // Get vendor
    std::string vendor = data.value("vendor", "generic");

    // Get features
    std::set<std::string> features;
    if (data.contains("features")) {
        for (const auto& f : data["features"]) {
            features.insert(f.get<std::string>());
        }
    }

    // Get compilers
    std::map<std::string, std::vector<CompilerEntry>> compilers;
    if (data.contains("compilers")) {
        for (auto it = data["compilers"].begin(); it != data["compilers"].end(); ++it) {
            std::vector<CompilerEntry> entries;
            for (const auto& entry : it.value()) {
                CompilerEntry ce;
                ce.versions = entry.value("versions", ":");
                ce.name = entry.value("name", "");
                ce.flags = entry.value("flags", "");
                ce.warnings = entry.value("warnings", "");
                entries.push_back(ce);
            }
            compilers[it.key()] = entries;
        }
    }

    // Get generation (for POWER)
    int generation = data.value("generation", 0);

    // Get CPU part (for ARM)
    std::string cpu_part = data.value("cpupart", "");

    // Create the microarchitecture
    targets_[name] =
        Microarchitecture(name, parents, vendor, features, compilers, generation, cpu_part);
}

void MicroarchitectureDatabase::load_embedded_data() {
    load_from_string(MICROARCHITECTURES_JSON);
}

} // namespace archspec
