#pragma once

#include <string>
#include <unordered_map>

namespace lsp::document {

class DocumentManager {
private:
  std::unordered_map<std::string, std::string> documents;

public:
  void open_document(const std::string &uri, const std::string &text) {
    documents[uri] = text;
  }

  void update_document(const std::string &uri, const std::string &text) {
    documents[uri] = text;
  }

  void close_document(const std::string &uri) { documents.erase(uri); }

  std::string get_document(const std::string &uri) const {
    auto it = documents.find(uri);
    if (it != documents.end())
      return it->second;
    return "";
  }
};

} // namespace lsp::document
