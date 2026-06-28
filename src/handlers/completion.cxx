#include "lib/server.hxx"
#include "lib/utils/protocol.hxx"
#include "lib/utils/schema.hxx"
#include "parsers/json.hxx"
#include <string>

using json = nlohmann::json;

// -----------------------------------------------------------------------
// NOTE: Helpers
// -----------------------------------------------------------------------

// Figure out what table path the cursor is sitting in by scanning upward
// through the lines looking for the most recent [section] or [[section]] header
static std::string get_table_context(const std::string &content,
                                     int cursor_line) {
  std::vector<std::string> lines;
  std::istringstream stream(content);
  std::string line;
  while (std::getline(stream, line))
    lines.push_back(line);

  std::string current_table;
  for (int i = 0; i <= cursor_line && i < (int)lines.size(); ++i) {
    const std::string &l = lines[i];
    // trim leading whitespace
    size_t start = l.find_first_not_of(" \t");
    if (start == std::string::npos)
      continue;
    std::string trimmed = l.substr(start);

    // [[array_of_tables]]
    if (trimmed.size() >= 4 && trimmed[0] == '[' && trimmed[1] == '[') {
      size_t end = trimmed.find("]]");
      if (end != std::string::npos)
        current_table = trimmed.substr(2, end - 2);
      continue;
    }
    // [table]
    if (trimmed[0] == '[') {
      size_t end = trimmed.find(']');
      if (end != std::string::npos)
        current_table = trimmed.substr(1, end - 1);
      continue;
    }
  }
  return current_table;
}

// NOTE: Normalize a raw header like "target.my_engine_core" -> "target.*"
// or "target.my_engine_core.sources_if" -> "target.*.sources_if"
static std::string
normalize_path(const std::string &raw,
               const std::vector<mokai::schema::TableDef> &schema) {
  // exact match first
  for (const auto &t : schema)
    if (t.path == raw)
      return raw;

  // try wildcard on last segment
  auto dot = raw.rfind('.');
  if (dot != std::string::npos) {
    std::string w1 = raw.substr(0, dot) + ".*";
    for (const auto &t : schema)
      if (t.path == w1)
        return w1;

    // two-level wildcard
    auto dot2 = raw.substr(0, dot).rfind('.');
    if (dot2 != std::string::npos) {
      std::string w2 = raw.substr(0, dot2) + ".*." + raw.substr(dot + 1);
      for (const auto &t : schema)
        if (t.path == w2)
          return w2;
    }
  }
  return raw; // unknown, return as-is
}

// INFO: Get what the user has typed on the current line (partial key or value)
static std::string get_current_line(const std::string &content,
                                    int cursor_line) {
  std::istringstream stream(content);
  std::string line;
  int i = 0;
  while (std::getline(stream, line)) {
    if (i == cursor_line)
      return line;
    ++i;
  }
  return "";
}

// Check if cursor is on the right side of '=' (completing a value)
static bool is_value_context(const std::string &line, int cursor_col) {
  std::string before = line.substr(0, cursor_col);
  return before.find('=') != std::string::npos;
}

// Extract the key name from a line like "  cpp_version = "
static std::string extract_key(const std::string &line) {
  size_t eq = line.find('=');
  if (eq == std::string::npos)
    return "";
  std::string key = line.substr(0, eq);
  // trim
  size_t s = key.find_first_not_of(" \t");
  size_t e = key.find_last_not_of(" \t");
  if (s == std::string::npos)
    return "";
  return key.substr(s, e - s + 1);
}

// LSP completion item kinds
// 10 = Property, 12 = Value, 14 = Keyword, 5 = Field
static json make_field_item(const mokai::schema::FieldDef &f, int index) {
  std::string insert;
  switch (f.type) {
  case mokai::schema::FieldType::String:
    insert = f.allowed_values.empty()
                 ? f.key + " = \"\""
                 : f.key + " = \"" + f.allowed_values[0] + "\"";
    break;
  case mokai::schema::FieldType::Bool:
    insert = f.key + " = true";
    break;
  case mokai::schema::FieldType::Array:
    insert = f.key + " = []";
    break;
  case mokai::schema::FieldType::Table:
    insert = f.key + " = {}";
    break;
  case mokai::schema::FieldType::ArrayOfTables:
    insert = f.key + " = []";
    break;
  }

  std::string detail = f.description;
  if (!f.allowed_values.empty()) {
    detail += " (";
    for (size_t i = 0; i < f.allowed_values.size(); ++i) {
      if (i)
        detail += ", ";
      detail += f.allowed_values[i];
    }
    detail += ")";
  }
  if (f.required)
    detail = "[required] " + detail;

  char sort[8];
  std::snprintf(sort, sizeof(sort), "%04d", index);

  return {
      {"label", f.key},
      {"kind", f.required ? 14 : 10}, // Keyword if required, Property otherwise
      {"detail", detail},
      {"insertText", insert},
      {"sortText", std::string(sort)},
      {"documentation", f.description}};
}

static bool cur_at_end(std::string line, int col) {
  // End of line checker
  std::string after = line.substr(col);
  return after.find_first_not_of(" \t") == std::string::npos;
  // return col >= (int)line.size();
}

static bool is_line_complete(const std::string &line, int col) {

  size_t eq = line.find('=');
  if (eq == std::string::npos)
    return false;

  std::string after_eq = line.substr(eq + 1);
  // trim leading whitespace after =
  size_t s = after_eq.find_first_not_of(" \t");
  if (s == std::string::npos)
    return false;

  std::string val = after_eq.substr(s);

  // complete quoted string: starts and ends with matching quote
  if ((val.front() == '"' || val.front() == '\'') && val.size() >= 2) {
    char q = val.front();
    // find closing quote after the opening
    size_t close = val.find(q, 1);
    if (close != std::string::npos && (int)(eq + 1 + s + close) < col == false)
      return true;
  }

  // complete boolean
  if (val.rfind("true", 0) == 0 || val.rfind("false", 0) == 0)
    return true;

  // complete array: starts with [ and ends with ]
  if (val.front() == '[' && val.back() == ']')
    return true;

  return false;
}

static json make_value_item(const std::string &val, int index,
                            const std::string &cur_line, int cursor_col) {
  char sort[8];
  std::snprintf(sort, sizeof(sort), "%04d", index);

  std::string before = cur_line.substr(0, cursor_col);
  std::string after = cur_line.substr(cursor_col);

  // detect opening quote type
  char open_quote = 0;
  for (int i = (int)before.size() - 1; i >= 0; --i) {
    if (before[i] == '"' || before[i] == '\'') {
      open_quote = before[i];
      break;
    }
    if (before[i] == '=')
      break; // hit = before finding a quote, no open quote
  }

  // detect if closing quote already exists after cursor
  bool has_close = !after.empty() && (after[0] == '"' || after[0] == '\'');

  std::string st = (open_quote == 0) ? "\"" : ""; // add opening if none
  std::string end =
      has_close ? "" : (open_quote ? std::string(1, open_quote) : "\"");

  return {{"label", val},
          {"kind", 13},
          {"insertText", st + val + end},
          {"filterText", val},
          {"sortText", std::string(sort)}};
}

static bool is_new_field_line(const std::string &line, int col) {
  std::string before = line.substr(0, col);
  return before.find_first_not_of(" \t") == std::string::npos;
}

// -----------------------------------------------------------------------
// NOTE: Handler
// -----------------------------------------------------------------------

void lsp::server::LspServer::handle_completion(const nlohmann::json &msg) {
  protocol::Protocol::progress_begin("p_completions", "Completions", "Started");

  auto items = json::array();
  auto schema = mokai::schema::get_schema();

  std::string uri = msg["params"]["textDocument"]["uri"];
  int cursor_line = msg["params"]["position"]["line"];
  int cursor_col = msg["params"]["position"]["character"];
  std::string content = doc_manager.get_document(uri);

  protocol::Protocol::progress_report("p_completions", "In Progress", 47);

  std::string raw_table = get_table_context(content, cursor_line);
  std::string table_path = normalize_path(raw_table, schema);
  std::string cur_line = get_current_line(content, cursor_line);

  const mokai::schema::TableDef *def =
      mokai::schema::find_table(schema, table_path);

  if (is_value_context(cur_line, cursor_col)) {
    // --- Value completion ---
    std::string key = extract_key(cur_line);
    // FIX: Checker if line is completed and cur at end to stop completion
    // if (!is_line_complete(cur_line, cursor_line) && !cur_at_end(cur_line,
    // cursor_col)

    if (def && !key.empty()) {
      for (const auto &f : def->fields) {
        if (f.key == key && !f.allowed_values.empty()) {
          int i = 0;
          for (const auto &val : f.allowed_values)
            items.push_back(make_value_item(val, i++, cur_line, cursor_col));
          break;
        }
      }
    }
  } else {
    // FIX: Checker for new lines for completion
    // if (is_new_field_line(cur_line, cursor_line))

    // --- Key completion ---
    if (def) {
      // suggest fields from the matched schema table
      int i = 0;
      for (const auto &f : def->fields)
        items.push_back(make_field_item(f, i++));
    } else if (raw_table.empty()) {
      // cursor is at root level — suggest top-level table headers
      std::vector<std::string> top_level = {
          "project", "options", "compatibility", "exports", "output"};
      std::vector<std::string> aot = {"file_group", "property_group", "hook"};
      int i = 0;
      for (const auto &t : top_level) {
        items.push_back(
            {{"label", "[" + t + "]"},
             {"kind", 14},
             {"insertText", "[" + t + "]"},
             {"detail", "Table: " + t},
             {"sortText", std::string(4 - (int)std::to_string(i).size(), '0') +
                              std::to_string(i)}});
        ++i;
      }
      for (const auto &t : aot) {
        items.push_back(
            {{"label", "[[" + t + "]]"},
             {"kind", 14},
             {"insertText", "[[" + t + "]]"},
             {"detail", "Array of tables: " + t},
             {"sortText", std::string(4 - (int)std::to_string(i).size(), '0') +
                              std::to_string(i)}});
        ++i;
      }
    }
  }

  protocol::Protocol::send_message(
      {{"jsonrpc", "2.0"},
       {"id", msg["id"]},
       {"result", {{"isIncomplete", false}, {"items", items}}}});

  protocol::Protocol::progress_end("p_completions", "Fetched Completions");
}
