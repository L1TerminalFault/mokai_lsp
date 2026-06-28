#pragma once

#include <exception>
#include <ios>
#include <iostream>
#include <string>

#include "../../parsers/json.hxx"

namespace lsp::protocol {

struct Protocol {
  static void init_io() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
  }

  static bool read_next_message(std::string &out_body) {
    std::string line;
    size_t content_length = 0;

    while (std::getline(std::cin, line)) {
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      if (line.empty())
        break;

      if (line.starts_with("Content-Length: "))
        try {
          content_length = std::stoull(line.substr(16));
        } catch (const std::exception &e) {
          return false;
        }
    }

    if (content_length == 0)
      return false;

    out_body.resize(content_length);
    std::cin.read(out_body.data(), content_length);

    if (std::cin.gcount() != static_cast<std::streamsize>(content_length))
      return false;

    return true;
  }

  static void send_message(const nlohmann::json &payload) {
    std::string body = payload.dump();

    std::string header =
        "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";

    std::string packet = header + body;

    std::cout.write(packet.data(), packet.size());
    std::cout.flush();
  }

  static void send_notification(const std::string &method,
                                const nlohmann::json &params) {
    nlohmann::json notification_packet = {
        {"jsonrpc", "2.0"}, {"method", method}, {"params", params}};
    send_message(notification_packet);
  }

  static void debug_log(const std::string &msg) {
    std::cerr << "[LSP-DEBUG] " << msg << std::endl;
  }

  // INFO: type: 1=Error, 2=Warning, 3=Info, 4=Log
  static void log_message(int type, const std::string &message) {
    nlohmann::json params = {
        {"type", type}, {"message", message}}; // FIXED: was {"verbose", type}
    send_notification("window/logMessage", params);
  }

  // INFO: Progresses
  static void progress_begin(const std::string &token, const std::string &title,
                             const std::string &message = "") {
    nlohmann::json params = {
        {"token", token},
        {"value", {{"kind", "begin"}, {"title", title}, {"message", message}}}};
    send_notification("$/progress", params);
  }

  static void progress_report(const std::string &token,
                              const std::string &message, double percentage) {
    nlohmann::json params = {{"token", token},
                             {"value",
                              {{"kind", "report"},
                               {"message", message},
                               {"percentage", percentage}}}};
    send_notification("$/progress", params);
  }

  static void progress_end(const std::string &token,
                           const std::string &message = "") {
    nlohmann::json params = {
        {"token", token}, {"value", {{"kind", "end"}, {"message", message}}}};
    send_notification("$/progress", params);
  }

  // BUG: These natively don't work
  // void show_message(int type, const std::string &message);
  // void info_message(const std::string &message);
  // void log_message(int type, const std::string &message);
  //
  // void send_notification(const std::string &method, const nlohmann::json
  // &params);
};

} // namespace lsp::protocol
