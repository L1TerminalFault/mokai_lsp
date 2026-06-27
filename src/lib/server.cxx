#include <iostream>

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

      if (method == "initialize") {
        handle_initialize(msg);
      } else if (method == "textDocument/didOpen") {
        handle_did_open(msg);
      } else if (method == "textDocument/didChange") {
        handle_did_change(msg);
      } else if (method == "textDocument/didSave") {
        handle_did_save(msg);
      } else if (method == "textDocument/documentHighlight") {
        handle_document_highlight(msg);
      } else if (method == "textDocument/signatureHelp") {
        handle_signature_help(msg);
      } else if (method == "textDocument/rename") {
        handle_rename(msg);
      } else if (method == "textDocument/formatting") {
        handle_formatting(msg);
      } else if (method == "textDocument/rangeFormatting") {
        handle_range_formatting(msg);
      } else if (method == "textDocument/foldingRange") {
        handle_folding_range(msg);
      } else if (method == "textDocument/selectionRange") {
        handle_selection_range(msg);
      } else if (method == "textDocument/hover") {
        handle_hover(msg);
      } else if (method == "textDocument/completion") {
        handle_completion(msg);
      } else if (method == "textDocument/definition") {
        handle_definition(msg);
      } else if (method == "textDocument/references") {
        handle_references(msg);
      } else if (method == "textDocument/documentSymbol") {
        handle_document_symbol(msg);
      } else if (method == "workspace/symbol") {
        handle_workspace_symbol(msg);
      } else if (method == "textDocument/didClose") {
        handle_did_close(msg);
      } else if (method == "shutdown") {
        handle_shutdown(msg);
      } else if (method == "exit") {
        handle_exit(msg);
      } else {
        protocol::Protocol::debug_log("Unhandled method: " + method);
      }
    } catch (const std::exception &e) {
      protocol::Protocol::debug_log("Exception in loop: " +
                                    std::string(e.what()));
    }
  }
}
