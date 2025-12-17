// This file is a part of Julia. License is MIT: https://julialang.org/license

#ifndef ARCHSPEC_MICROARCHITECTURE_HPP
#define ARCHSPEC_MICROARCHITECTURE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <cstdint>

namespace archspec {

// Forward declarations
class Microarchitecture;
class MicroarchitectureDatabase;

/**
 * Compiler information for a specific microarchitecture
 */
struct CompilerEntry {
    std::string versions; // Version range like "4.9:" or "3.9:11.1"
    std::string name;     // Optional compiler-specific name for the target
    std::string flags;    // Compiler flags to use
    std::string warnings; // Optional warning message
};

/**
 * Represents a CPU microarchitecture
 */
class Microarchitecture {
  public:
    Microarchitecture() = default;

    Microarchitecture(const std::string& name, const std::vector<std::string>& parents,
                      const std::string& vendor, const std::set<std::string>& features,
                      const std::map<std::string, std::vector<CompilerEntry>>& compilers,
                      int generation = 0, const std::string& cpu_part = "");

    // Accessors
    const std::string& name() const {
        return name_;
    }
    const std::string& vendor() const {
        return vendor_;
    }
    const std::set<std::string>& features() const {
        return features_;
    }
    const std::vector<std::string>& parent_names() const {
        return parent_names_;
    }
    const std::map<std::string, std::vector<CompilerEntry>>& compilers() const {
        return compilers_;
    }
    int generation() const {
        return generation_;
    }
    const std::string& cpu_part() const {
        return cpu_part_;
    }

    // Check if a feature is supported (includes aliases)
    bool has_feature(std::string_view feature) const;

    // Get all ancestors (parents and their parents recursively)
    std::vector<std::string> ancestors() const;

    // Get the architecture family (root ancestor)
    std::string family() const;

    // Get the best generic architecture compatible with this one
    std::string generic() const;

    // Comparison operators based on feature set hierarchy
    bool operator==(const Microarchitecture& other) const;
    bool operator!=(const Microarchitecture& other) const;
    bool operator<(const Microarchitecture& other) const; // this is subset of other
    bool operator<=(const Microarchitecture& other) const;
    bool operator>(const Microarchitecture& other) const; // other is subset of this
    bool operator>=(const Microarchitecture& other) const;

    // Get optimization flags for a compiler
    std::string optimization_flags(std::string_view compiler, std::string_view version) const;

    // Convert to/from string representation
    std::string to_string() const {
        return name_;
    }

    // Check if this is a valid/initialized microarchitecture
    bool valid() const {
        return !name_.empty();
    }

  private:
    std::string name_;
    std::vector<std::string> parent_names_;
    std::string vendor_;
    std::set<std::string> features_;
    std::map<std::string, std::vector<CompilerEntry>> compilers_;
    int generation_ = 0;
    std::string cpu_part_;

    // Helper to get all names in ancestor chain (including self)
    std::set<std::string> to_set() const;
};

/**
 * Database of all known microarchitectures
 * Singleton pattern with lazy initialization
 */
class MicroarchitectureDatabase {
  public:
    // Get the singleton instance
    static MicroarchitectureDatabase& instance();

    // Get a microarchitecture by name (returns nullopt if not found)
    std::optional<std::reference_wrapper<const Microarchitecture>> get(std::string_view name) const;

    // Check if a microarchitecture exists
    bool exists(std::string_view name) const;

    // Get all known microarchitecture names
    std::vector<std::string> all_names() const;

    // Get all microarchitectures
    const std::map<std::string, Microarchitecture>& all() const {
        return targets_;
    }

    // Load from various formats
    bool load_from_file(std::string_view path);
    bool load_from_string(std::string_view json_data);

    // Get feature aliases
    const std::map<std::string, std::set<std::string>>& feature_aliases() const {
        return feature_aliases_;
    }

    // Get family aliases (features implied by being part of a family)
    const std::map<std::string, std::set<std::string>>& family_features() const {
        return family_features_;
    }

    // Darwin to Linux flag conversions
    const std::map<std::string, std::string>& darwin_flag_conversions() const {
        return darwin_flags_;
    }

    // ARM vendor code to name mapping
    const std::map<std::string, std::string>& arm_vendors() const {
        return arm_vendors_;
    }

  private:
    MicroarchitectureDatabase();
    ~MicroarchitectureDatabase() = default;
    MicroarchitectureDatabase(const MicroarchitectureDatabase&) = delete;
    MicroarchitectureDatabase& operator=(const MicroarchitectureDatabase&) = delete;

    void load_embedded_data();

    std::map<std::string, Microarchitecture> targets_;
    std::map<std::string, std::set<std::string>> feature_aliases_;
    std::map<std::string, std::set<std::string>> family_features_;
    std::map<std::string, std::string> darwin_flags_;
    std::map<std::string, std::string> arm_vendors_;
    bool loaded_ = false;

    // Allow JSON parsing helper access to private members
    friend bool load_json_into_database(MicroarchitectureDatabase& db, std::string_view json_data);
};

// Convenience function to get a microarchitecture by name
inline std::optional<std::reference_wrapper<const Microarchitecture>>
get_target(std::string_view name) {
    return MicroarchitectureDatabase::instance().get(name);
}

// Create a generic microarchitecture with no features
Microarchitecture generic_microarchitecture(std::string_view name);

} // namespace archspec

#endif // ARCHSPEC_MICROARCHITECTURE_HPP
