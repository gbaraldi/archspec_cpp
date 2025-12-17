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

bool Microarchitecture::has_feature(std::string_view feature) const {
    std::string feature_str(feature);
    if (features_.count(feature_str))
        return true;

    const auto& db = MicroarchitectureDatabase::instance();

    if (auto it = db.feature_aliases().find(feature_str); it != db.feature_aliases().end()) {
        for (const auto& aliased : it->second) {
            if (features_.count(aliased))
                return true;
        }
    }

    if (auto it = db.family_features().find(feature_str); it != db.family_features().end()) {
        if (it->second.count(family()))
            return true;
    }

    return false;
}

std::vector<std::string> Microarchitecture::ancestors() const {
    std::vector<std::string> result;
    const auto& db = MicroarchitectureDatabase::instance();

    // Breadth-first: add direct parents first
    for (const auto& parent_name : parent_names_)
        result.push_back(parent_name);

    for (const auto& parent_name : parent_names_) {
        if (auto parent = db.get(parent_name)) {
            for (const auto& ancestor : parent->get().ancestors()) {
                if (std::find(result.begin(), result.end(), ancestor) == result.end())
                    result.push_back(ancestor);
            }
        }
    }

    return result;
}

std::string Microarchitecture::family() const {
    if (parent_names_.empty())
        return name_;

    const auto& db = MicroarchitectureDatabase::instance();
    std::vector<std::string> roots;

    for (const auto& ancestor_name : ancestors()) {
        if (auto ancestor = db.get(ancestor_name)) {
            if (ancestor->get().parent_names().empty()) {
                if (std::find(roots.begin(), roots.end(), ancestor_name) == roots.end())
                    roots.push_back(ancestor_name);
            }
        }
    }

    if (roots.size() == 1)
        return roots[0];

    return roots.empty() ? name_ : roots[0];
}

std::string Microarchitecture::generic() const {
    if (vendor_ == "generic")
        return name_;

    const auto& db = MicroarchitectureDatabase::instance();
    std::string best_generic;
    size_t best_depth = 0;

    for (const auto& ancestor_name : ancestors()) {
        if (auto ancestor = db.get(ancestor_name)) {
            if (ancestor->get().vendor() == "generic") {
                size_t depth = ancestor->get().ancestors().size();
                if (best_generic.empty() || depth > best_depth) {
                    best_generic = ancestor_name;
                    best_depth = depth;
                }
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
            // Parse without exceptions
            char* end = nullptr;
            long val = std::strtol(part.c_str(), &end, 10);
            if (end != part.c_str()) {
                result.push_back(static_cast<int>(val));
            }
            // Ignore non-numeric parts
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
bool satisfies_version(const std::string& constraint, std::string_view version) {
    size_t colon_pos = constraint.find(':');
    if (colon_pos == std::string::npos) {
        // No colon - exact match? (shouldn't happen in practice)
        return constraint == version;
    }

    std::string min_ver = constraint.substr(0, colon_pos);
    std::string max_ver = constraint.substr(colon_pos + 1);

    auto ver = parse_version(std::string(version));

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

std::string Microarchitecture::optimization_flags(std::string_view compiler,
                                                  std::string_view version) const {
    const auto& db = MicroarchitectureDatabase::instance();

    // Check if we have info for this compiler
    std::string compiler_str(compiler);
    auto it = compilers_.find(compiler_str);
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
        auto ancestor = db.get(ancestor_name);
        if (ancestor) {
            std::string result = ancestor->get().optimization_flags(compiler, version);
            if (!result.empty()) {
                return result;
            }
        }
    }

    return "";
}

Microarchitecture generic_microarchitecture(std::string_view name) {
    return Microarchitecture(std::string(name), {}, "generic", {}, {}, 0, "");
}

// MicroarchitectureDatabase implementation

MicroarchitectureDatabase& MicroarchitectureDatabase::instance() {
    static MicroarchitectureDatabase db;
    return db;
}

MicroarchitectureDatabase::MicroarchitectureDatabase() {
    load_embedded_data();
}

std::optional<std::reference_wrapper<const Microarchitecture>>
MicroarchitectureDatabase::get(std::string_view name) const {
    std::string name_str(name);
    auto it = targets_.find(name_str);
    if (it != targets_.end())
        return std::cref(it->second);
    return std::nullopt;
}

bool MicroarchitectureDatabase::exists(std::string_view name) const {
    return targets_.count(std::string(name)) > 0;
}

std::vector<std::string> MicroarchitectureDatabase::all_names() const {
    std::vector<std::string> names;
    names.reserve(targets_.size());
    for (const auto& [name, _] : targets_) {
        names.push_back(name);
    }
    return names;
}

bool MicroarchitectureDatabase::load_from_file(std::string_view path) {
    std::string path_str(path);
    std::ifstream file(path_str);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_string(buffer.str());
}

namespace {

void fill_target_from_json(std::map<std::string, Microarchitecture>& targets,
                           const std::string& name, const nlohmann::json& data) {
    if (targets.count(name))
        return;

    std::vector<std::string> parents;
    if (data.contains("from")) {
        for (const auto& parent : data["from"])
            parents.push_back(parent.get<std::string>());
    }

    std::set<std::string> features;
    if (data.contains("features")) {
        for (const auto& f : data["features"])
            features.insert(f.get<std::string>());
    }

    std::map<std::string, std::vector<CompilerEntry>> compilers;
    if (data.contains("compilers")) {
        for (auto it = data["compilers"].begin(); it != data["compilers"].end(); ++it) {
            std::vector<CompilerEntry> entries;
            for (const auto& entry : it.value()) {
                entries.push_back({entry.value("versions", ":"), entry.value("name", ""),
                                   entry.value("flags", ""), entry.value("warnings", "")});
            }
            compilers[it.key()] = entries;
        }
    }

    targets[name] =
        Microarchitecture(name, parents, data.value("vendor", "generic"), features, compilers,
                          data.value("generation", 0), data.value("cpupart", ""));
}

} // anonymous namespace

bool load_json_into_database(MicroarchitectureDatabase& db, std::string_view json_data) {
    nlohmann::json j = nlohmann::json::parse(json_data, nullptr, false);
    if (j.is_discarded())
        return false;

    if (j.contains("microarchitectures")) {
        for (auto it = j["microarchitectures"].begin(); it != j["microarchitectures"].end(); ++it)
            fill_target_from_json(db.targets_, it.key(), it.value());
    }

    if (j.contains("feature_aliases")) {
        for (auto it = j["feature_aliases"].begin(); it != j["feature_aliases"].end(); ++it) {
            const auto& alias_data = it.value();
            if (alias_data.contains("any_of")) {
                std::set<std::string> features;
                for (const auto& f : alias_data["any_of"])
                    features.insert(f.get<std::string>());
                db.feature_aliases_[it.key()] = features;
            }
            if (alias_data.contains("families")) {
                std::set<std::string> families;
                for (const auto& f : alias_data["families"])
                    families.insert(f.get<std::string>());
                db.family_features_[it.key()] = families;
            }
        }
    }

    if (j.contains("conversions")) {
        const auto& conv = j["conversions"];
        if (conv.contains("darwin_flags")) {
            for (auto it = conv["darwin_flags"].begin(); it != conv["darwin_flags"].end(); ++it)
                db.darwin_flags_[it.key()] = it.value().get<std::string>();
        }
        if (conv.contains("arm_vendors")) {
            for (auto it = conv["arm_vendors"].begin(); it != conv["arm_vendors"].end(); ++it)
                db.arm_vendors_[it.key()] = it.value().get<std::string>();
        }
    }

    db.loaded_ = true;
    return true;
}

bool MicroarchitectureDatabase::load_from_string(std::string_view json_data) {
    return load_json_into_database(*this, json_data);
}

void MicroarchitectureDatabase::load_embedded_data() {
    load_from_string(MICROARCHITECTURES_JSON);
}

} // namespace archspec
