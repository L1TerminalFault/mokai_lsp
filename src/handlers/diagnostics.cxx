#include "../lib/server.hxx"
#include "../lib/utils/protocol.hxx"
#include "../lib/utils/schema.hxx"
#include "../parsers/json.hxx"
#include "../parsers/toml.hxx"

using json = nlohmann::json;

// INFO: Helpers
static json make_diagnostic(int line, int col, int end_col, int severity,
                            const std::string &msg) {
  return {{"range",
           {{"start", {{"line", line}, {"character", col}}},
            {"end", {{"line", line}, {"character", end_col}}}}},
          {"severity", severity}, // 1=error 2=warn 3=info 4=hint
          {"source", "MokaiLSP"},
          {"message", msg}};
}

// Converts a toml::source_position to 0-based line/col
static std::pair<int, int> toml_pos(const toml::source_position &p) {
  return {(int)p.line - 1, (int)p.column - 1};
}

// Build a dot-separated table path from a toml::path or manual key stack
static std::string join_path(const std::vector<std::string> &parts) {
  std::string out;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i)
      out += '.';
    out += parts[i];
  }
  return out;
}

// INFO: Recursive walker
static void walk_table(const toml::table &tbl,
                       const std::vector<std::string> &path_parts,
                       const std::vector<mokai::schema::TableDef> &schema,
                       json &diags) {

  std::string path = join_path(path_parts);
  const mokai::schema::TableDef *def = mokai::schema::find_table(schema, path);

  // [options] is special: all keys must be booleans, any key name is allowed
  bool is_options = (path == "options");

  for (const auto &[key, node] : tbl) {
    std::string key_str(key.str());
    auto [line, col] = toml_pos(node.source().begin);
    int end_col = col + (int)key_str.size();

    // --- options block: only booleans allowed ---
    if (is_options) {
      if (node.type() != toml::node_type::boolean) {
        diags.push_back(make_diagnostic(
            line, col, end_col, 1,
            "options." + key_str + ": value must be a boolean (true/false)."));
      }
      continue;
    }

    // --- unknown table path (no schema entry at all) ---
    if (!def) {
      // only warn at top-level unknown tables; nested ones caught by parent
      if (path_parts.size() <= 1) {
        diags.push_back(make_diagnostic(line, col, end_col, 2,
                                        "Unknown top-level key or table: '" +
                                            key_str + "'."));
      }
      // still recurse so inner nodes get checked if we find a schema later
      if (node.is_table()) {
        auto child_parts = path_parts;
        child_parts.push_back(key_str);
        walk_table(*node.as_table(), child_parts, schema, diags);
      }
      continue;
    }

    // --- look up field in the table's schema ---
    const mokai::schema::FieldDef *field_def = nullptr;
    for (const auto &f : def->fields) {
      if (f.key == key_str) {
        field_def = &f;
        break;
      }
    }

    if (!field_def) {
      diags.push_back(make_diagnostic(line, col, end_col, 1,
                                      "Unknown field '" + key_str + "' in [" +
                                          path +
                                          "]. "
                                          "Not part of the Mokai spec."));
      continue;
    }

    // --- type checking ---
    bool type_ok = false;
    std::string expected_type;
    switch (field_def->type) {
    case mokai::schema::FieldType::String:
      type_ok = node.is_string();
      expected_type = "string";
      break;
    case mokai::schema::FieldType::Bool:
      type_ok = node.is_boolean();
      expected_type = "boolean";
      break;
    case mokai::schema::FieldType::Array:
      type_ok = node.is_array();
      expected_type = "array";
      break;
    case mokai::schema::FieldType::Table:
      type_ok = node.is_table();
      expected_type = "table";
      break;
    case mokai::schema::FieldType::ArrayOfTables:
      type_ok = node.is_array();
      expected_type = "array of tables";
      break;
    }

    if (!type_ok) {
      diags.push_back(make_diagnostic(line, col, end_col, 1,
                                      "Field '" + key_str + "' expects a " +
                                          expected_type + "."));
      continue;
    }

    // --- enum / allowed_values checking ---
    if (!field_def->allowed_values.empty() && node.is_string()) {
      std::string val = node.as_string()->get();
      bool found = false;
      for (const auto &av : field_def->allowed_values)
        if (av == val) {
          found = true;
          break;
        }

      if (!found) {
        std::string allowed_list;
        for (size_t i = 0; i < field_def->allowed_values.size(); ++i) {
          if (i)
            allowed_list += ", ";
          allowed_list += "'" + field_def->allowed_values[i] + "'";
        }
        diags.push_back(make_diagnostic(line, col, end_col, 1,
                                        "Invalid value '" + val + "' for '" +
                                            key_str +
                                            "'. "
                                            "Allowed: " +
                                            allowed_list + "."));
      }
    }

    // --- recurse into sub-tables ---
    if (node.is_table()) {
      auto child_parts = path_parts;
      child_parts.push_back(key_str);
      walk_table(*node.as_table(), child_parts, schema, diags);
    }
  }

  // --- required field checking ---
  if (def) {
    for (const auto &f : def->fields) {
      if (!f.required)
        continue;
      if (!tbl.contains(f.key)) {
        // point to line 0 col 0 as a fallback; toml++ doesn't give table
        // position easily
        auto src = tbl.source();
        auto [line, col] = toml_pos(src.begin);
        diags.push_back(make_diagnostic(line, col, col + (int)f.key.size(), 2,
                                        "Missing required field '" + f.key +
                                            "' in [" + path + "]."));
      }
    }
  }
}

// INFO: Array of tables walker
static void
walk_array_of_tables(const toml::table &root, const std::string &key,
                     const std::vector<mokai::schema::TableDef> &schema,
                     json &diags) {
  auto *node = root.get(key);
  if (!node || !node->is_array())
    return;

  for (const auto &elem : *node->as_array()) {
    if (!elem.is_table())
      continue;
    walk_table(*elem.as_table(), {key}, schema, diags);
  }
}

// INFO: Entry
void lsp::server::LspServer::validate_toml(const std::string &uri,
                                           const std::string content) {
  protocol::Protocol::progress_begin("p_diagnostics", "Diagnostics",
                                     "Diagnosing Syntax");

  json diags = json::array();
  auto schema = mokai::schema::get_schema();

  toml::table root;
  try {
    root = toml::parse(content);
  } catch (const toml::parse_error &err) {
    auto [line, col] = toml_pos(err.source().begin);
    diags.push_back(make_diagnostic(
        line, col, col + 2, 1, "Syntax: " + std::string(err.description())));
    // publish and bail — no point schema-checking malformed TOML
    protocol::Protocol::send_message(
        {{"jsonrpc", "2.0"},
         {"method", "textDocument/publishDiagnostics"},
         {"params", {{"uri", uri}, {"diagnostics", diags}}}});
    return;
  }

  // Walk known standard tables
  for (const std::string &top :
       {"project", "compatibility", "exports", "output", "options"}) {
    auto *node = root.get(top);
    if (!node || !node->is_table())
      continue;
    walk_table(*node->as_table(), {top}, schema, diags);
  }

  // compatibility.compilers subtable
  if (auto *compat = root.get("compatibility"); compat && compat->is_table()) {
    if (auto *compilers = compat->as_table()->get("compilers");
        compilers && compilers->is_table()) {
      walk_table(*compilers->as_table(), {"compatibility", "compilers"}, schema,
                 diags);
    }
  }

  // output.configs.* subtables
  if (auto *out = root.get("output"); out && out->is_table()) {
    if (auto *configs = out->as_table()->get("configs");
        configs && configs->is_table()) {
      for (const auto &[cfg_key, cfg_node] : *configs->as_table()) {
        if (cfg_node.is_table()) {
          walk_table(*cfg_node.as_table(),
                     {"output", "configs", std::string(cfg_key.str())}, schema,
                     diags);
        }
      }
    }
  }

  // exports.profile.* subtables
  if (auto *exp = root.get("exports"); exp && exp->is_table()) {
    if (auto *prof = exp->as_table()->get("profile");
        prof && prof->is_table()) {
      for (const auto &[pkey, pnode] : *prof->as_table()) {
        if (pnode.is_table()) {
          walk_table(*pnode.as_table(),
                     {"exports", "profile", std::string(pkey.str())}, schema,
                     diags);
        }
      }
    }
  }

  // [target.*] and its nested _if array-of-tables
  if (auto *targets = root.get("target"); targets && targets->is_table()) {
    for (const auto &[tname, tnode] : *targets->as_table()) {
      std::string tname_str(tname.str());
      if (!tnode.is_table())
        continue;
      const auto &ttbl = *tnode.as_table();

      // validate top-level target fields
      walk_table(ttbl, {"target", tname_str}, schema, diags);

      // validate _if array-of-tables inside the target
      for (const std::string &if_key :
           {"sources_if", "flags_if", "properties_if"}) {
        auto *if_node = ttbl.get(if_key);
        if (!if_node || !if_node->is_array())
          continue;
        for (const auto &elem : *if_node->as_array()) {
          if (!elem.is_table())
            continue;
          walk_table(*elem.as_table(), {"target", tname_str, if_key}, schema,
                     diags);
        }
      }
    }
  }

  protocol::Protocol::progress_report("p_diagnostics", "Diagnosing", 49);

  // array-of-tables: [[file_group]], [[property_group]], [[hook]]
  walk_array_of_tables(root, "file_group", schema, diags);
  walk_array_of_tables(root, "property_group", schema, diags);
  walk_array_of_tables(root, "hook", schema, diags);

  protocol::Protocol::send_message(
      {{"jsonrpc", "2.0"},
       {"method", "textDocument/publishDiagnostics"},
       {"params", {{"uri", uri}, {"diagnostics", diags}}}});

  protocol::Protocol::progress_end("p_diagnostics", "Done Diagnosing");
}
