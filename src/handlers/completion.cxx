#include "lib/server.hxx"
#include "lib/utils/protocol.hxx"
#include "lib/utils/schema.hxx"
#include "parsers/json.hxx"
#include <string>
#include <string_view>

using json = nlohmann::json;

static std::string_view trim_left(std::string_view v) {
  size_t s = v.find_first_not_of(" \t");
  return (s == std::string_view::npos) ? "" : v.substr(s);
}

static std::string_view get_line_view(const std::string &content,
                                      int target_line) {
  size_t cur = 0;
  int line_idx = 0;
  while (cur < content.size()) {
    size_t next = content.find('\n', cur);
    size_t end = (next == std::string::npos) ? content.size() : next;
    if (line_idx == target_line) {
      return std::string_view(content.data() + cur, end - cur);
    }
    if (next == std::string::npos)
      break;
    cur = next + 1;
    ++line_idx;
  }
  return "";
}

static std::string get_table_context(const std::string &content,
                                     int cursor_line) {
  size_t cur = 0;
  int line_idx = 0;
  std::string last_table;
  while (cur < content.size() && line_idx <= cursor_line) {
    size_t next = content.find('\n', cur);
    size_t end = (next == std::string::npos) ? content.size() : next;
    std::string_view l =
        trim_left(std::string_view(content.data() + cur, end - cur));
    if (!l.empty()) {
      if (l.size() >= 4 && l[0] == '[' && l[1] == '[') {
        size_t e = l.find("]]");
        if (e != std::string_view::npos)
          last_table = std::string(l.substr(2, e - 2));
      } else if (l[0] == '[') {
        size_t e = l.find(']');
        if (e != std::string_view::npos)
          last_table = std::string(l.substr(1, e - 1));
      }
    }
    if (next == std::string::npos)
      break;
    cur = next + 1;
    ++line_idx;
  }
  return last_table;
}

static std::string
normalize_path(const std::string &raw,
               const std::vector<mokai::schema::TableDef> &schema) {
  for (const auto &t : schema)
    if (t.path == raw)
      return raw;
  size_t dot = raw.rfind('.');
  if (dot != std::string::npos) {
    std::string w1 = raw.substr(0, dot) + ".*";
    for (const auto &t : schema)
      if (t.path == w1)
        return w1;
    size_t dot2 = raw.substr(0, dot).rfind('.');
    if (dot2 != std::string::npos) {
      std::string w2 = raw.substr(0, dot2) + ".*." + raw.substr(dot + 1);
      for (const auto &t : schema)
        if (t.path == w2)
          return w2;
    }
  }
  return raw;
}

static inline std::string make_sort_text(int index) {
  char buf[8];
  std::snprintf(buf, sizeof(buf), "%04d", index);
  return std::string(buf);
}

static json make_field_item(const mokai::schema::FieldDef &f, int index) {
  std::string insert;
  switch (f.type) {
  case mokai::schema::FieldType::String:
    insert = f.allowed_values.empty()
                 ? f.key + " = \"$1\""
                 : f.key + " = \"" + f.allowed_values[0] + "\"";
    break;
  case mokai::schema::FieldType::Bool:
    insert = f.key + " = ${1:true}";
    break;
  case mokai::schema::FieldType::Array:
    insert = f.key + " = [$1]";
    break;
  case mokai::schema::FieldType::Table:
    insert = f.key + " = {$1}";
    break;
  case mokai::schema::FieldType::ArrayOfTables:
    insert = f.key + " = [$1]";
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
  return {{"label", f.key},
          {"kind", f.required ? 14 : 10},
          {"detail", detail},
          {"insertText", insert},
          {"insertTextFormat", 2},
          {"sortText", make_sort_text(index)},
          {"documentation", f.description}};
}

static json make_value_item(const std::string &val, int index,
                            std::string_view before, std::string_view after) {
  char open_quote = 0;
  for (int i = (int)before.size() - 1; i >= 0; --i) {
    if (before[i] == '"' || before[i] == '\'') {
      open_quote = before[i];
      break;
    }
    if (before[i] == '=')
      break;
  }
  bool has_close = !after.empty() && (after[0] == '"' || after[0] == '\'');
  std::string st = (open_quote == 0) ? "\"" : "";
  std::string end =
      has_close ? "" : (open_quote ? std::string(1, open_quote) : "\"");
  return {{"label", val},
          {"kind", 13},
          {"insertText", st + val + end},
          {"filterText", val},
          {"sortText", make_sort_text(index)}};
}

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
  std::string_view cur_line = get_line_view(content, cursor_line);
  const mokai::schema::TableDef *def =
      mokai::schema::find_table(schema, table_path);
  std::string_view before = (cursor_col <= (int)cur_line.size())
                                ? cur_line.substr(0, cursor_col)
                                : cur_line;
  std::string_view after = (cursor_col < (int)cur_line.size())
                               ? cur_line.substr(cursor_col)
                               : std::string_view();

  if (before.find('=') != std::string_view::npos) {
    size_t eq = cur_line.find('=');
    std::string_view key_part =
        (eq != std::string_view::npos) ? cur_line.substr(0, eq) : "";
    std::string_view trimmed_key = trim_left(key_part);
    while (!trimmed_key.empty() &&
           (trimmed_key.back() == ' ' || trimmed_key.back() == '\t')) {
      trimmed_key.remove_suffix(1);
    }
    std::string key(trimmed_key);

    if (def && !key.empty()) {
      for (const auto &f : def->fields) {
        if (f.key == key) {
          // If it has predefined values, suggest them explicitly
          if (!f.allowed_values.empty()) {
            int i = 0;
            for (const auto &val : f.allowed_values) {
              items.push_back(make_value_item(val, i++, before, after));
            }
          } else if (f.type == mokai::schema::FieldType::String) {
            bool has_quotes = (before.find('"') != std::string_view::npos ||
                               after.find('"') != std::string_view::npos);
            if (!has_quotes) {
              items.push_back({{"label", "\"\""},
                               {"kind", 12},
                               {"insertText", "\"$0\""},
                               {"insertTextFormat", 2},
                               {"detail", "Empty string value"}});
            }
          }
          break;
        }
      }
    }
  } else {
    if (def) {
      int i = 0;
      for (const auto &f : def->fields)
        items.push_back(make_field_item(f, i++));
    } else if (raw_table.empty()) {
      std::vector<std::string> top_level = {
          "project", "target", "options",      "compatibility",
          "exports", "output", "dependencies", "build"};
      std::vector<std::string> aot = {"file_group", "property_group", "hook",
                                      "env", "test"};
      int i = 0;
      for (const auto &t : top_level) {
        items.push_back({{"label", "[" + t + "]"},
                         {"kind", 14},
                         {"insertText", "[" + t + "]\n$0"},
                         {"insertTextFormat", 2},
                         {"detail", "Table: " + t},
                         {"sortText", make_sort_text(i++)}});
      }
      for (const auto &t : aot) {
        items.push_back({{"label", "[[" + t + "]]"},
                         {"kind", 14},
                         {"insertText", "[[" + t + "]]\n$0"},
                         {"insertTextFormat", 2},
                         {"detail", "Array of tables: " + t},
                         {"sortText", make_sort_text(i++)}});
      }
    }
  }
  protocol::Protocol::send_message(
      {{"jsonrpc", "2.0"},
       {"id", msg["id"]},
       {"result", {{"isIncomplete", false}, {"items", items}}}});
  protocol::Protocol::progress_end("p_completions", "Fetched Completions");
}
