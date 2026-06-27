#include "../lib/server.hxx"
#include "../lib/utils/protocol.hxx"
#include "../lib/utils/schema.hxx"
#include "../parsers/json.hxx"
#include <sstream>

using json = nlohmann::json;

// INFO: Helpers
static std::string hover_get_line(const std::string &content, int target_line) {
  std::istringstream stream(content);
  std::string line;
  int i = 0;
  while (std::getline(stream, line)) {
    if (i == target_line)
      return line;
    ++i;
  }
  return "";
}

static std::string hover_get_table_context(const std::string &content,
                                           int cursor_line) {
  std::istringstream stream(content);
  std::string line;
  std::string current_table;
  int i = 0;
  while (std::getline(stream, line)) {
    if (i > cursor_line)
      break;
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) {
      ++i;
      continue;
    }
    std::string trimmed = line.substr(start);
    if (trimmed.size() >= 4 && trimmed[0] == '[' && trimmed[1] == '[') {
      size_t end = trimmed.find("]]");
      if (end != std::string::npos)
        current_table = trimmed.substr(2, end - 2);
    } else if (trimmed[0] == '[') {
      size_t end = trimmed.find(']');
      if (end != std::string::npos)
        current_table = trimmed.substr(1, end - 1);
    }
    ++i;
  }
  return current_table;
}

// INFO: Extract the word under the cursor from a line
static std::string get_word_at(const std::string &line, int col) {
  if (col >= (int)line.size())
    col = (int)line.size() - 1;
  if (col < 0 || line.empty())
    return "";

  // find start
  int s = col;
  while (s > 0 && (std::isalnum(line[s - 1]) || line[s - 1] == '_'))
    --s;
  // find end
  int e = col;
  while (e < (int)line.size() && (std::isalnum(line[e]) || line[e] == '_'))
    ++e;

  return line.substr(s, e - s);
}

// INFO: Is the cursor on the left side of '=' (hovering a key, not a value)
static bool is_key_side(const std::string &line, int col) {
  size_t eq = line.find('=');
  if (eq == std::string::npos)
    return true;
  return (size_t)col < eq;
}

// INFO: Is this line a table header [ ] or [[ ]]
static bool is_table_header(const std::string &line) {
  size_t s = line.find_first_not_of(" \t");
  if (s == std::string::npos)
    return false;
  return line[s] == '[';
}

static std::string
normalize_hover_path(const std::string &raw,
                     const std::vector<mokai::schema::TableDef> &schema) {
  for (const auto &t : schema)
    if (t.path == raw)
      return raw;
  auto dot = raw.rfind('.');
  if (dot != std::string::npos) {
    std::string w1 = raw.substr(0, dot) + ".*";
    for (const auto &t : schema)
      if (t.path == w1)
        return w1;
    auto dot2 = raw.substr(0, dot).rfind('.');
    if (dot2 != std::string::npos) {
      std::string w2 = raw.substr(0, dot2) + ".*." + raw.substr(dot + 1);
      for (const auto &t : schema)
        if (t.path == w2)
          return w2;
    }
  }
  return raw;
}

// INFO: Build the markdown hover content for a field
static std::string field_hover_md(const std::string &table_path,
                                  const mokai::schema::FieldDef &f) {
  std::string type_str;
  switch (f.type) {
  case mokai::schema::FieldType::String:
    type_str = "string";
    break;
  case mokai::schema::FieldType::Bool:
    type_str = "boolean";
    break;
  case mokai::schema::FieldType::Array:
    type_str = "array";
    break;
  case mokai::schema::FieldType::Table:
    type_str = "inline table";
    break;
  case mokai::schema::FieldType::ArrayOfTables:
    type_str = "array of tables";
    break;
  }

  std::string md;
  md += "### `" + f.key + "`\n\n";
  md += f.description + "\n\n";
  md += "---\n\n";
  md += "**Table**     `[" + table_path + "]`  \n";
  md += "**Type**      `" + type_str + "`  \n";
  md += "**Required**  " + std::string(f.required ? "Yes" : "") + "  \n";

  if (!f.allowed_values.empty()) {
    md += "\n---\n\n";
    md += "**Allowed values**\n\n";
    for (const auto &v : f.allowed_values)
      md += "- `\"" + v + "\"`\n";
  }

  return md;
}

static std::string pad_right(const std::string &str, size_t total_len) {
  if (str.length() >= total_len)
    return str;
  return str + std::string(total_len - str.length(), ' ');
}

// INFO: Build hover for a table header itself
static std::string table_hover_md(const mokai::schema::TableDef &def) {
  std::string md;
  md += "### `[" + def.path + "]`\n";
  md += def.description + "\n\n";

  if (!def.fields.empty()) {

    size_t max_key_len = std::string("Field").length();
    size_t max_type_len = std::string("Type").length();
    size_t max_req_len = std::string("Required").length();

    struct FormattedRow {
      std::string key, type, req;
    };
    std::vector<FormattedRow> rows;

    for (const auto &f : def.fields) {
      std::string type_str;
      switch (f.type) {
      case mokai::schema::FieldType::String:
        type_str = "string";
        break;
      case mokai::schema::FieldType::Bool:
        type_str = "boolean";
        break;
      case mokai::schema::FieldType::Array:
        type_str = "array";
        break;
      case mokai::schema::FieldType::Table:
        type_str = "table";
        break;
      case mokai::schema::FieldType::ArrayOfTables:
        type_str = "array[]";
        break;
      default:
        type_str = "unknown";
        break;
      }

      std::string fmt_key = "*" + f.key + "*";
      std::string fmt_type = "`" + type_str + "`";
      std::string fmt_req = f.required ? "Yes" : "";

      max_key_len = std::max(max_key_len, fmt_key.length());
      max_type_len = std::max(max_type_len, fmt_type.length());
      max_req_len = std::max(max_req_len, fmt_req.length());

      rows.push_back({fmt_key, fmt_type, fmt_req});
    }

    md += "  " + pad_right("**Field**", max_key_len) + "     " +
          pad_right("**Type**", max_type_len) + "     " +
          pad_right("**Required**", max_req_len) + "  \n\n";

    md += "---\n\n";

    for (const auto &row : rows) {
      md += "  " + pad_right(row.key, max_key_len) + "   " +
            pad_right(row.type, max_type_len) + "   " +
            pad_right(row.req, max_req_len) + "  \n";
    }
    md += "\n";
  }

  return md;
}

// -----------------------------------------------------------------------
// Handler
// -----------------------------------------------------------------------

void lsp::server::LspServer::handle_hover(const json &msg) {
  protocol::Protocol::progress_begin("p_tooltips", "Tooltips",
                                     "Fetching Tooltips");

  std::string uri = msg["params"]["textDocument"]["uri"];
  int line = msg["params"]["position"]["line"];
  int col = msg["params"]["position"]["character"];
  std::string content = doc_manager.get_document(uri);
  auto schema = mokai::schema::get_schema();

  std::string cur_line = hover_get_line(content, line);
  std::string raw_table = hover_get_table_context(content, line);
  std::string table_path = normalize_hover_path(raw_table, schema);
  const mokai::schema::TableDef *def =
      mokai::schema::find_table(schema, table_path);

  protocol::Protocol::progress_report("p_tooltips", "Fetching Tooltips", 47);

  std::string md;

  if (is_table_header(cur_line)) {
    // hovering over a [section] line — show table-level docs
    std::string word = get_word_at(cur_line, col);
    // try to find the full table path from the header text
    size_t s = cur_line.find_first_not_of(" \t");
    std::string header = cur_line.substr(s);
    // strip [[ ]] or [ ]
    if (header.size() >= 4 && header[1] == '[') {
      size_t e = header.find("]]");
      if (e != std::string::npos)
        raw_table = header.substr(2, e - 2);
    } else if (header[0] == '[') {
      size_t e = header.find(']');
      if (e != std::string::npos)
        raw_table = header.substr(1, e - 1);
    }
    table_path = normalize_hover_path(raw_table, schema);
    const mokai::schema::TableDef *hdef =
        mokai::schema::find_table(schema, table_path);
    if (hdef)
      md = table_hover_md(*hdef);

  } else if (def && is_key_side(cur_line, col)) {
    // hovering over a key name
    std::string word = get_word_at(cur_line, col);
    for (const auto &f : def->fields) {
      if (f.key == word) {
        md = field_hover_md(table_path, f);
        break;
      }
    }
    // unknown key — still show something useful
    if (md.empty() && !word.empty())
      md = "`" + word + "` — unknown field in `[" + table_path + "]`";

  } else if (def && !is_key_side(cur_line, col)) {
    // hovering over a value — show the field docs too
    std::string key = "";
    size_t eq = cur_line.find('=');
    if (eq != std::string::npos) {
      std::string left = cur_line.substr(0, eq);
      size_t ks = left.find_first_not_of(" \t");
      size_t ke = left.find_last_not_of(" \t");
      if (ks != std::string::npos)
        key = left.substr(ks, ke - ks + 1);
    }
    for (const auto &f : def->fields) {
      if (f.key == key) {
        md = field_hover_md(table_path, f);
        break;
      }
    }
  }

  if (md.empty()) {
    // null result = no hover popup
    protocol::Protocol::send_message(
        {{"jsonrpc", "2.0"}, {"id", msg["id"]}, {"result", nullptr}});
    return;
  }

  protocol::Protocol::send_message(
      {{"jsonrpc", "2.0"},
       {"id", msg["id"]},
       {"result", {{"contents", {{"kind", "markdown"}, {"value", md}}}}}});

  protocol::Protocol::progress_end("p_tooltips", "Tooltips Feched");
}
