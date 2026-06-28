#include "lib/server.hxx"
#include "lib/utils/protocol.hxx"
#include "parsers/json.hxx"

using json = nlohmann::json;

// INFO: Extra features just incase they need to be implemented (filled with
// logs for now not to hold the editor);
void lsp::server::LspServer::handle_definition(const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_references(const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_document_symbol(const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_workspace_symbol(
    const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_did_close(const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_shutdown(const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_exit(const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_document_highlight(
    const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_signature_help(const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_rename(const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_folding_range(const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};

void lsp::server::LspServer::handle_selection_range(const nlohmann::json &msg) {
  protocol::Protocol::log_message(4, "definition");
};
