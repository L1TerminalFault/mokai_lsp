#pragma once

#include "../parsers/json.hxx"
#include "utils/document.hxx"

namespace lsp::server {

class LspServer {
private:
  lsp::document::DocumentManager doc_manager;
  bool initialized = false;

  void handle_initialize(const nlohmann::json &msg);
  void handle_did_open(const nlohmann::json &msg);
  void handle_did_change(const nlohmann::json &msg);
  void handle_did_close(const nlohmann::json &msg);
  void handle_did_save(const nlohmann::json &msg);
  void handle_shutdown(const nlohmann::json &msg);
  void handle_exit(const nlohmann::json &msg);
  void handle_hover(const nlohmann::json &msg);
  void handle_completion(const nlohmann::json &msg);
  void handle_definition(const nlohmann::json &msg);
  void handle_references(const nlohmann::json &msg);
  void handle_document_symbol(const nlohmann::json &msg);
  void handle_workspace_symbol(const nlohmann::json &msg);
  void handle_code_action(const nlohmann::json &msg);
  void handle_code_lens(const nlohmann::json &msg);
  void handle_document_highlight(const nlohmann::json &msg);
  void handle_document_link(const nlohmann::json &msg);
  void handle_document_color(const nlohmann::json &msg);
  void handle_document_formatting(const nlohmann::json &msg);
  void handle_document_range_formatting(const nlohmann::json &msg);
  void handle_document_on_type_formatting(const nlohmann::json &msg);
  void handle_rename(const nlohmann::json &msg);
  void handle_folding_range(const nlohmann::json &msg);
  void handle_selection_range(const nlohmann::json &msg);
  void handle_signature_help(const nlohmann::json &msg);
  void handle_formatting(const nlohmann::json &msg);
  void handle_range_formatting(const nlohmann::json &msg);
  void handle_linked_editing_range(const nlohmann::json &msg);
  void handle_call_hierarchy(const nlohmann::json &msg);
  void handle_inlay_hint(const nlohmann::json &msg);
  void handle_moniker(const nlohmann::json &msg);
  void handle_type_definition(const nlohmann::json &msg);
  void handle_implementation(const nlohmann::json &msg);
  void handle_declaration(const nlohmann::json &msg);
  void handle_semantic_tokens(const nlohmann::json &msg);
  void handle_code_lens_refresh(const nlohmann::json &msg);
  void handle_workspace_did_change_configuration(const nlohmann::json &msg);
  void handle_workspace_did_change_watched_files(const nlohmann::json &msg);
  void handle_workspace_execute_command(const nlohmann::json &msg);
  void handle_workspace_symbol_refresh(const nlohmann::json &msg);
  void handle_workspace_apply_edit(const nlohmann::json &msg);
  void handle_workspace_did_create_files(const nlohmann::json &msg);
  void handle_workspace_did_rename_files(const nlohmann::json &msg);
  void handle_workspace_did_delete_files(const nlohmann::json &msg);
  void handle_workspace_will_create_files(const nlohmann::json &msg);
  void handle_workspace_will_rename_files(const nlohmann::json &msg);
  void handle_workspace_will_delete_files(const nlohmann::json &msg);
  void handle_workspace_did_change_workspace_folders(const nlohmann::json &msg);
  void handle_workspace_did_change_workspace_configuration(
      const nlohmann::json &msg);
  void handle_workspace_did_change_workspace_symbol(const nlohmann::json &msg);
  void handle_workspace_did_change_workspace_symbol_refresh(
      const nlohmann::json &msg);

  void validate_toml(const std::string &uri, const std::string content);

public:
  void run();
};

} // namespace lsp::server
