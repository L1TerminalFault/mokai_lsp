#include "lib/server.hxx"
#include "lib/utils/protocol.hxx"
#include "parsers/json.hxx"

using json = nlohmann::json;

const json &triggerCharacters = {".", "=", "[", " ", "\"",
                                 "'", "+", "@", "/", "_"};

void lsp::server::LspServer::handle_initialize(const json &msg) {
  json response = {
      {"jsonrpc", "2.0"},
      {"id", msg["id"]},
      {"result",
       {{"capabilities",
         {{"textDocumentSync",
           {{"openClose", true}, {"change", 1}, {"save", true}}},

          {"completionProvider",
           {{"resolveProvider", false},
            {"triggerCharacters", triggerCharacters}}},

          {"hoverProvider", true},
          {"definitionProvider", true},
          {"referencesProvider", true},
          {"documentSymbolProvider", true},
          {"workspaceSymbolProvider", true},
          {"renameProvider", true},
          {"documentFormattingProvider", true},
          {"documentRangeFormattingProvider", true},
          {"documentHighlightProvider", true},
          {"foldingRangeProvider", true},
          {"selectionRangeProvider", true},

          {"signatureHelpProvider", {{"triggerCharacters", {"(", ","}}}}}}}}};
  protocol::Protocol::send_message(response);
  initialized = true;
}
