#include <functional>
#include <iostream>
#include <unordered_map>

#include "server.hxx"
#include "utils/protocol.hxx"

using json = nlohmann::json;

void lsp::server::LspServer::run() {
  protocol::Protocol::init_io();
  protocol::Protocol::debug_log("Mokai TOML LSP Engine online.");

  std::string raw_body;
  while (std::cin.good()) {
    if (!protocol::Protocol::read_next_message(raw_body))
      continue;

    try {
      json msg = json::parse(raw_body);
      protocol::Protocol::debug_log(msg.dump(2));

      std::string method = msg.value("method", "");
      protocol::Protocol::debug_log("Method: " + method);

      auto it = handlers.find(method);
      if (it != handlers.end())
        it->second(msg);
      else
        protocol::Protocol::debug_log("Unhandled method: " + method);
    } catch (const std::exception &e) {
      protocol::Protocol::debug_log("Exception in loop: " +
                                    std::string(e.what()));
    }
  }
}
