#include "lib/server.hxx"
#include "lib/utils/protocol.hxx"

void lsp::server::LspServer::handle_formatting(const nlohmann::json &msg) {
  protocol::Protocol::send_message(
      {{"jsonrpc", "2.0"}, {"id", msg["id"]}, {"result", json::array()}});
};

void lsp::server::LspServer::handle_range_formatting(
    const nlohmann::json &msg) {
  protocol::Protocol::send_message(
      {{"jsonrpc", "2.0"}, {"id", msg["id"]}, {"result", json::array()}});
};
