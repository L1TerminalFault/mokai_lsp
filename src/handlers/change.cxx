#include "../lib/server.hxx"
#include "../parsers/json.hxx"

using json = nlohmann::json;

void lsp::server::LspServer::handle_did_change(const json &msg) {
  std::string uri = msg["params"]["textDocument"]["uri"];
  std::string text = msg["params"]["contentChanges"][0]["text"];

  // INFO: Sync memory state
  doc_manager.update_document(uri, text);

  // INFO: Run validation using our helper function
  validate_toml(uri, text);
}
