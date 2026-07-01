#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace mokai::schema {

enum class FieldType { String, Bool, Array, Table, ArrayOfTables };

struct FieldDef {
  std::string key;
  FieldType type;
  bool required = false;
  std::string description;
  std::vector<std::string> allowed_values;
};

struct TableDef {
  std::string path;
  std::vector<FieldDef> fields;
  std::string description;
};

inline std::vector<TableDef> get_schema() {
  return {
      {"project",
       {{"name", FieldType::String, true, "Unique identifier package token."},
        {"version", FieldType::String, false,
         "Fallback project version literal."},
        {"license", FieldType::String, false,
         "SPDX license metadata identifier."},
        {"description", FieldType::String, false,
         "Detailed package overview text."},
        {"homepage", FieldType::String, false, "Project website landing URL."},
        {"cpp_version",
         FieldType::String,
         false,
         "C++ standard baseline dialect.",
         {"c++11", "c++14", "c++17", "c++20", "c++23", "c++26"}},
        {"default_compiler",
         FieldType::String,
         false,
         "Fallback system toolchain driver select.",
         {"g++", "clang++", "msvc"}},
        {"version_from", FieldType::Table, false,
         "Dynamic rule regex file tracker layout: { file, pattern }."},
        {"authors", FieldType::Array, false, "List of project author strings."},
        {"include_dirs", FieldType::Array, false,
         "Global include paths appended downstream."},
        {"dependencies", FieldType::Array, false,
         "Package array upstream requirements declarations."}},
       "Root package matrix."},
      {"options", {}, "User option flag toggles validation map."},
      {"compatibility",
       {{"min_cpp_version",
         FieldType::String,
         false,
         "Minimum toolchain standard barrier.",
         {"c++11", "c++14", "c++17", "c++20", "c++23", "c++26"}},
        {"preferred_cpp_version",
         FieldType::String,
         false,
         "Target standard runtime optimization.",
         {"c++11", "c++14", "c++17", "c++20", "c++23", "c++26"}},
        {"unsupported_cpp_versions", FieldType::Array, false,
         "Blacklisted compiler standards list."}},
       "Toolchain constraints engine validation arrays."},
      {"compatibility.compilers",
       {{"supported",
         FieldType::Array,
         false,
         "Allowed host systems toolchains list.",
         {"g++", "clang++", "msvc"}},
        {"unsupported",
         FieldType::Array,
         false,
         "Banned host systems toolchains list.",
         {"g++", "clang++", "msvc"}}},
       "Compiler validation checks layout."},
      {"file_group",
       {{"name", FieldType::String, true,
         "Logical reference alias identifier."},
        {"patterns", FieldType::Array, true,
         "File system glob strings arrays lookup matching."}},
       "File system asset clusters arrays descriptors."},
      {"property_group",
       {{"name", FieldType::String, true,
         "Reference identification array string token."},
        {"condition", FieldType::String, false,
         "Conditional logic block matching engine string."},
        {"defines", FieldType::Array, false,
         "Preprocessor macro definitions arrays injections."},
        {"flags", FieldType::Array, false,
         "Hardcoded tool parameter modifications arrays lists."}},
       "Shared compile parameters target configurations profiles."},
      {"hook",
       {{"name", FieldType::String, true,
         "Callback identity tracking key label."},
        {"on",
         FieldType::String,
         true,
         "Pipeline interception execution state trigger step.",
         {"pre_build", "post_build", "pre_target_build", "post_target_build",
          "file_change"}},
        {"run", FieldType::String, true,
         "Shell executable string statement invocation text."},
        {"target", FieldType::String, false,
         "Target scope isolation filter match string."},
        {"pattern", FieldType::String, false,
         "Monitored watch file system glob string matches."}},
       "Lifecycle pipeline automation state triggers block."},
      {"target.*",
       {{"type",
         FieldType::String,
         true,
         "Output compilation artifact binary layer layout format.",
         {"executable", "static_library", "shared_library"}},
        {"sources", FieldType::Array, false,
         "Explicit file lists mapping parameters targets."},
        {"include_dirs", FieldType::Array, false,
         "Isolated search paths directories mapping parameters."},
        {"properties", FieldType::Array, false,
         "Profile link array identifier tokens (@name lookup)."},
        {"flags",
         FieldType::Array,
         false,
         "Hardcoded compiler parameters optimizations levels modifiers lists.",
         {"-O0", "-O1", "-O2", "-O3", "-Os", "-Og", "-g"}},
        {"system_libs", FieldType::Array, false,
         "Linker framework external binary system libraries link strings."},
        {"depends_on", FieldType::Array, false,
         "Internal task nodes DAG topology dependencies array map link "
         "references."}},
       "Compilation units build generation layout attributes."},
      {"target.*.sources_if",
       {{"condition", FieldType::String, true,
         "Evaluation condition expression script verification statement."},
        {"patterns", FieldType::Array, true,
         "Conditional dynamic layout file system source globs match targets."}},
       "Conditional asset matching layout modifiers rules."},
      {"target.*.flags_if",
       {{"condition", FieldType::String, true,
         "Evaluation condition expression script verification statement."},
        {"flags", FieldType::Array, true,
         "Conditional tool optimization parameter flags definitions arrays "
         "strings."}},
       "Conditional parameters optimization levels rules attributes "
       "modifiers."},
      {"target.*.properties_if",
       {{"condition", FieldType::String, false,
         "Evaluation condition expression script verification statement."},
        {"defines", FieldType::Array, false,
         "Conditional micro symbol preprocessor injections arrays attributes."},
        {"depends_on", FieldType::Array, false,
         "Conditional topology dependencies graphs arrays structures "
         "references."}},
       "Conditional profile attribute fields mapping rules modifiers "
       "configurations."},
      {"exports",
       {{"default_targets", FieldType::Array, true,
         "Default visible artifact build pipeline links target mappings "
         "strings."},
        {"include_dirs", FieldType::Array, false,
         "Public include distributions interface paths rules lookup roots."},
        {"defines_required", FieldType::Array, false,
         "Downstream mandatory validation macro tracking markers injections."},
        {"defines_optional", FieldType::Array, false,
         "Downstream interface generic macro definitions parameter "
         "configurations arrays."}},
       "Public downstream API packaging integration rules distribution "
       "definitions bindings."},
      {"exports.profile.*",
       {{"targets", FieldType::Array, true,
         "Profile scope specific packaging visibility array paths target "
         "links."}},
       "Export package grouping profile target clusters mappings layout "
       "rules."},
      {"output",
       {{"directory", FieldType::String, false,
         "Root build compilation output artifact directory workspace file "
         "root."}},
       "Compilation generation system storage path layouts options."},
      {"output.configs.*",
       {{"enabled", FieldType::Bool, false,
         "Variant status selection condition execution tracking toggle "
         "parameter."},
        {"subdir", FieldType::String, false,
         "Output target variant compilation directory workspace subdirectory "
         "mapping configuration text."}},
       "Compilation mode variants subdirectories layout configurations options "
       "mapping variables declarations."},
      {"dependencies",
       {},
       "External package resolution trees layout parameters settings tracking "
       "blocks."},
      {"build",
       {{"parallel_jobs", FieldType::String, false,
         "Thread limit execution constraint counts configurations."},
        {"generator",
         FieldType::String,
         false,
         "Target compilation tool generation engine choice selection.",
         {"ninja", "make", "internal"}},
        {"verbose", FieldType::Bool, false,
         "Pipeline standard output telemetry string verbosity flags state "
         "parameter."}},
       "Custom pipeline build environment runtime optimization settings "
       "attributes engine configurations."},
      {"env",
       {},
       "Local execution platform environment key override assignments mappings "
       "definitions."},
      {"test",
       {{"command", FieldType::String, true,
         "Test framework shell execution runner text string."},
        {"timeout", FieldType::String, false,
         "Maximum execution loop safety time barrier limits configuration."},
        {"targets", FieldType::Array, false,
         "Scope validation specific test suite execution target dependencies "
         "lists structures."}},
       "Integrated test validation block specifications automation matrix "
       "options mapping configurations definitions."}};
}

inline const TableDef *find_table(const std::vector<TableDef> &schema,
                                  std::string_view path) {
  for (const auto &t : schema)
    if (t.path == path)
      return &t;
  size_t dot = path.rfind('.');
  if (dot != std::string_view::npos) {
    std::string w1 = std::string(path.substr(0, dot)) + ".*";
    for (const auto &t : schema)
      if (t.path == w1)
        return &t;
    size_t dot2 = path.substr(0, dot).rfind('.');
    if (dot2 != std::string_view::npos) {
      std::string w2 = std::string(path.substr(0, dot2)) + ".*." +
                       std::string(path.substr(dot + 1));
      for (const auto &t : schema)
        if (t.path == w2)
          return &t;
    }
  }
  return nullptr;
}

} // namespace mokai::schema
