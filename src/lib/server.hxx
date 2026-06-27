#pragma once

#include "../parsers/json.hxx"
#include "utils/document.hxx"

using json = nlohmann::json;

namespace lsp::server {

class LspServer {
private:
  lsp::document::DocumentManager doc_manager;
  bool initialized = false;

  void handle_initialize(const json &msg);
  void handle_did_open(const json &msg);
  void handle_did_change(const json &msg);
  void handle_did_close(const json &msg);
  void handle_did_save(const json &msg);
  void handle_shutdown(const json &msg);
  void handle_exit(const json &msg);
  void handle_hover(const json &msg);
  void handle_completion(const json &msg);
  void handle_definition(const json &msg);
  void handle_references(const json &msg);
  void handle_document_symbol(const json &msg);
  void handle_workspace_symbol(const json &msg);
  void handle_code_action(const json &msg);
  void handle_code_lens(const json &msg);
  void handle_document_highlight(const json &msg);
  void handle_document_link(const json &msg);
  void handle_document_color(const json &msg);
  void handle_document_formatting(const json &msg);
  void handle_document_range_formatting(const json &msg);
  void handle_document_on_type_formatting(const json &msg);
  void handle_rename(const json &msg);
  void handle_folding_range(const json &msg);
  void handle_selection_range(const json &msg);
  void handle_signature_help(const json &msg);
  void handle_formatting(const json &msg);
  void handle_range_formatting(const json &msg);
  void handle_linked_editing_range(const json &msg);
  void handle_call_hierarchy(const json &msg);
  void handle_inlay_hint(const json &msg);
  void handle_moniker(const json &msg);
  void handle_type_definition(const json &msg);
  void handle_implementation(const json &msg);
  void handle_declaration(const json &msg);
  void handle_semantic_tokens(const json &msg);
  void handle_code_lens_refresh(const json &msg);
  void handle_workspace_did_change_configuration(const json &msg);
  void handle_workspace_did_change_watched_files(const json &msg);
  void handle_workspace_execute_command(const json &msg);
  void handle_workspace_symbol_refresh(const json &msg);
  void handle_workspace_apply_edit(const json &msg);
  void handle_workspace_did_create_files(const json &msg);
  void handle_workspace_did_rename_files(const json &msg);
  void handle_workspace_did_delete_files(const json &msg);
  void handle_workspace_will_create_files(const json &msg);
  void handle_workspace_will_rename_files(const json &msg);
  void handle_workspace_will_delete_files(const json &msg);
  void handle_workspace_did_change_workspace_folders(const json &msg);
  void handle_workspace_did_change_workspace_configuration(const json &msg);
  void handle_workspace_did_change_workspace_symbol(const json &msg);
  void handle_workspace_did_change_workspace_symbol_refresh(const json &msg);

  void validate_toml(const std::string &uri, const std::string content);

  std::unordered_map<std::string, std::function<void(const json &msg)>>
      handlers = {
          {"initialize", [&](auto &msg) { handle_initialize(msg); }},
          {"textDocument/didChange",
           [&](auto &msg) { handle_did_change(msg); }},
          {"textDocument/didOpen", [&](auto &msg) { handle_did_open(msg); }},
          {"textDocument/didSave", [&](auto &msg) { handle_did_save(msg); }},
          {"textDocument/documentHighlight",
           [&](auto &msg) { handle_document_highlight(msg); }},
          {"textDocument/signatureHelp",
           [&](auto &msg) { handle_signature_help(msg); }},
          {"textDocument/rename", [&](auto &msg) { handle_rename(msg); }},
          {"textDocument/formatting",
           [&](auto &msg) { handle_formatting(msg); }},
          {"textDocument/rangeFormatting",
           [&](auto &msg) { handle_range_formatting(msg); }},
          {"textDocument/foldingRange",
           [&](auto &msg) { handle_folding_range(msg); }},
          {"textDocument/selectionRange",
           [&](auto &msg) { handle_selection_range(msg); }},
          {"textDocument/hover", [&](auto &msg) { handle_hover(msg); }},
          // } else if (method == "textDocument/completion") {
          {"textDocument/completion",
           [&](auto &msg) { handle_completion(msg); }},
          {"textDocument/references",
           [&](auto &msg) { handle_references(msg); }},
          {"textDocument/definition",
           [&](auto &msg) { handle_definition(msg); }},
          {"textDocument/documentSymbol",
           [&](auto &msg) { handle_document_symbol(msg); }},
          {"textDocument/symbol",
           [&](auto &msg) { handle_workspace_symbol(msg); }},
          {"textDocument/didClose", [&](auto &msg) { handle_did_close(msg); }},
          {"textDocument/shutdown", [&](auto &msg) { handle_shutdown(msg); }},
          {"textDocument/exit", [&](auto &msg) { handle_exit(msg); }},
  };

public:
  void run();
};

} // namespace lsp::server
