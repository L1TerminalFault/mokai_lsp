#include "lib/server.hxx"

int main() {
  lsp::server::LspServer server;
  server.run();
  return 0;
}
