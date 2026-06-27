#pragma once

#include <string>
#include <vector>

namespace mokai::schema {

enum class FieldType {
  String,
  Bool,
  Array,
  Table,
  ArrayOfTables,
};

struct FieldDef {
  std::string key;
  FieldType type;
  bool required = false;
  std::string description;
  std::vector<std::string> allowed_values; // empty = any value ok
};

struct TableDef {
  std::string path; // e.g. "project", "target.*", "target.*.sources_if"
  std::vector<FieldDef> fields;
  std::string description;
};

// INFO: Completely constructed by an agent (let's be real who the F#CK deals
// with this)

inline std::vector<TableDef> get_schema() {
  return {
      {"project",
       {
           {"name", FieldType::String, true,
            "Unique string token identifying the package."},
           {"version", FieldType::String, false,
            "Baseline version fallback literal."},
           {"license", FieldType::String, false, "License metadata."},
           {"description", FieldType::String, false, "Project description."},
           {"homepage", FieldType::String, false, "Homepage URL."},
           {"cpp_version",
            FieldType::String,
            false,
            "C++ standard version.",
            {"c++11", "c++14", "c++17", "c++20", "c++23", "c++26"}},
           {"version_from", FieldType::Table, false,
            "Dynamic versioning rule: { file, pattern }."},
           {"authors", FieldType::Array, false, "List of author names."},
           {"include_dirs", FieldType::Array, false,
            "Global include paths appended to all targets."},
           {"dependencies", FieldType::Array, false,
            "Local (`./`) or remote (`git:`) dependencies."},
       },
       "Root project manifest block."},
      {"options",
       {
           // options keys are user-defined booleans — validated dynamically
       },
       "Project-level boolean option toggles. All values must be booleans."},
      {"compatibility",
       {
           {"min_cpp_version",
            FieldType::String,
            false,
            "Minimum required C++ version.",
            {"c++11", "c++14", "c++17", "c++20", "c++23", "c++26"}},
           {"preferred_cpp_version",
            FieldType::String,
            false,
            "Preferred C++ version.",
            {"c++11", "c++14", "c++17", "c++20", "c++23", "c++26"}},
           {"unsupported_cpp_versions", FieldType::Array, false,
            "List of unsupported C++ versions."},
       },
       "Compatibility gating matrix."},
      {"compatibility.compilers",
       {
           {"supported",
            FieldType::Array,
            false,
            "Supported compilers.",
            {"clang", "gcc", "msvc"}},
           {"unsupported",
            FieldType::Array,
            false,
            "Unsupported compilers.",
            {"clang", "gcc", "msvc"}},
       },
       "Compiler compatibility constraints."},
      {"file_group", // array of tables
       {
           {"name", FieldType::String, true,
            "Logical name for this file group."},
           {"patterns", FieldType::Array, true,
            "Glob patterns for source files."},
       },
       "Logical file group alias (array of tables)."},
      {"property_group",
       {
           {"name", FieldType::String, true,
            "Name used to reference via '@' prefix."},
           {"condition", FieldType::String, false,
            "Condition expression for activation."},
           {"defines", FieldType::Array, false, "Preprocessor defines."},
           {"flags", FieldType::Array, false, "Compiler flags."},
       },
       "Reusable property group referenced in targets."},
      {"hook",
       {
           {"name", FieldType::String, true, "Hook identifier name."},
           {"on",
            FieldType::String,
            true,
            "Lifecycle trigger event.",
            {"pre_build", "post_build", "pre_target_build", "post_target_build",
             "file_change"}},
           {"run", FieldType::String, true, "Shell command to execute."},
           {"target", FieldType::String, false,
            "Restrict hook to a single target."},
           {"pattern", FieldType::String, false,
            "Glob pattern for file_change tracking."},
       },
       "Lifecycle interceptor hook."},
      {"target.*",
       {
           {"type",
            FieldType::String,
            true,
            "Target output type.",
            {"executable", "static_library", "shared_library"}},
           {"sources", FieldType::Array, false, "Static source file list."},
           {"include_dirs", FieldType::Array, false,
            "Target-scoped include paths."},
           {"properties", FieldType::Array, false,
            "Property group references (use '@name')."},
           {"flags", FieldType::Array, false, "Hardcoded compiler flags."},
           {"system_libs", FieldType::Array, false,
            "System libraries to link."},
           {"depends_on", FieldType::Array, false,
            "Target or package dependencies."},
       },
       "Compilation target definition."},
      {"target.*.sources_if",
       {
           {"condition", FieldType::String, true, "Condition expression."},
           {"patterns", FieldType::Array, true,
            "Glob patterns for conditional sources."},
       },
       "Conditional source file modifier."},
      {"target.*.flags_if",
       {
           {"condition", FieldType::String, true, "Condition expression."},
           {"flags", FieldType::Array, true, "Conditional compiler flags."},
       },
       "Conditional flags modifier."},
      {"target.*.properties_if",
       {
           {"condition", FieldType::String, false, "Condition expression."},
           {"defines", FieldType::Array, false, "Conditional defines."},
           {"depends_on", FieldType::Array, false, "Conditional dependencies."},
       },
       "Conditional properties modifier."},
      {"exports",
       {
           {"default_targets", FieldType::Array, true,
            "Targets linked by default on package match."},
           {"include_dirs", FieldType::Array, false,
            "Public include directories."},
           {"defines_required", FieldType::Array, false,
            "Defines consumers must set."},
           {"defines_optional", FieldType::Array, false,
            "Defines consumers may set."},
       },
       "Interface distribution layer for downstream consumers."},
      {"exports.profile.*",
       {
           {"targets", FieldType::Array, true,
            "Targets included in this export profile."},
       },
       "Named export profile cluster."},
      {"output",
       {
           {"directory", FieldType::String, false, "Base output directory."},
       },
       "Artifact generation layout."},
      {"output.configs.*",
       {
           {"enabled", FieldType::Bool, false,
            "Whether this config is active."},
           {"subdir", FieldType::String, false,
            "Subdirectory for this config's output."},
       },
       "Per-config output settings (debug, release, etc)."},
  };
}

// Lookup a TableDef by exact path or wildcard match (e.g.
// "target.my_engine_core" -> "target.*")
inline const TableDef *find_table(const std::vector<TableDef> &schema,
                                  const std::string &path) {
  // exact match first
  for (const auto &t : schema)
    if (t.path == path)
      return &t;

  // wildcard match: replace last segment with *
  auto dot = path.rfind('.');
  if (dot != std::string::npos) {
    std::string wildcard = path.substr(0, dot) + ".*";
    for (const auto &t : schema)
      if (t.path == wildcard)
        return &t;

    // two-level wildcard e.g. target.my_engine_core.sources_if ->
    // target.*.sources_if
    auto dot2 = path.substr(0, dot).rfind('.');
    if (dot2 != std::string::npos) {
      std::string w2 = path.substr(0, dot2) + ".*." + path.substr(dot + 1);
      for (const auto &t : schema)
        if (t.path == w2)
          return &t;
    }
  }
  return nullptr;
}

} // namespace mokai::schema
