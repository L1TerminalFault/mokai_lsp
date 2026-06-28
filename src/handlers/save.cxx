#include "lib/server.hxx"
#include "lib/utils/protocol.hxx"

void lsp::server::LspServer::handle_did_save(const nlohmann::json &msg) {
  protocol::Protocol::progress_begin("p_save", "Saving");
  protocol::Protocol::progress_end("p_save", "Saved");
};
