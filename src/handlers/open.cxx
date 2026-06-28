#include "lib/server.hxx"
#include "lib/utils/protocol.hxx"
#include "parsers/json.hxx"

using json = nlohmann::json;

void lsp::server::LspServer::handle_did_open(const nlohmann::json &msg) {
  protocol::Protocol::progress_begin("p_open", "Mokai LSP", "Initializing");

  std::string uri = msg["params"]["textDocument"]["uri"];
  std::string text = msg["params"]["textDocument"]["text"];
  doc_manager.open_document(uri, text);

  protocol::Protocol::progress_end("p_open", std::string("Opened `") + uri +
                                                 std::string("`"));

  validate_toml(uri, text);
};
