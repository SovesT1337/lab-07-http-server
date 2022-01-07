#include <example.hpp>
#include <lambda.hpp>
#include <nlohmann/json.hpp>

using nlohmann::json;

auto from_json(const json& j) -> std::string {
    return j.get<std::string>();
}

template <class Body, class Allocator, class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>>&& req,
                    Send&& send) {

  auto const bad_request = [&req](beast::string_view why) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req.version()};
    res.body() = string(why);
    return res;
  };

  boost::beast::string_view content_type = req[http::field::content_type];
  if (content_type != "application/json" || req.target() != "suggest") {
    return send(bad_request("Bad request"));
  }

  http::response<http::string_body> res{http::status::ok, req.version()};

  json jtext = json::parse(req.body());
  string input_value = from_json(jtext.at("input"));

  try {
    std::ifstream file("../v1/api/suggest/file.json");
    json data;
    file >> data;
    std::map<size_t, json> cost;
    for (auto& sug : data.at("requests").at(input_value)) {
      json request_arr = json::object();
      request_arr["text"] = sug["name"];
      cost[sug["cost"]] = request_arr;
    }

    json request = json::object();
    size_t i = 0;
    for (auto& elem : cost) {
      elem.second["position"] = i;
      request["suggestions"].push_back(elem.second);
      ++i;
    }
    res.body() = request.dump();
  } catch (const std::exception& ex) {
    if (std::string(ex.what()) ==
        ("[json.exception.out_of_range.403] key '" + input_value  + "' not found")) {
      std::ifstream dictionary("/usr/share/dict/words");
      std::string s;
      std::regex reg("\\b(" + input_value + ")([^ ]*)");

      size_t i = 0;
      std::vector<std::string> sug;
      while (std::getline(dictionary, s) && i < 4) {
        std::smatch m;
        while (std::regex_search(s, m, reg) && i < 4) {
          sug.push_back(m.str());
          s = m.suffix().str();
          ++i;
        }
      }
      sug.erase(sug.begin());
      json request = json::object();
      size_t pos = 0;
      std::ifstream file("../v1/api/suggest/file.json");
      json data = json::object();
      file >> data;
      file.close();
      for (auto& elem : sug) {
        json new_sug_arr = json::object();
        json request_arr = json::object();
        new_sug_arr["name"] = elem;
        request_arr["text"] = elem;
        new_sug_arr["cost"] = (10 - int(pow(2, pos + 1))) * 100;
        request_arr["position"] = pos;
        ++pos;
        data["requests"][input_value].push_back(new_sug_arr);
        request["request"].push_back(request_arr);
      }
      std::ofstream ofile("../v1/api/suggest/file.json");
      ofile << data;
      ofile.close();
      res.body() = request.dump();
    }
  }
  res.prepare_payload();
  return send(std::move(res));
}

// Report a failure
void fail(beast::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

void do_session(tcp::socket& socket) {
  bool close = false;
  beast::error_code ec;
  beast::flat_buffer buffer;
  send_lambda<tcp::socket> lambda{socket, close, ec};
  for (;;) {
    http::request<http::string_body> req;
    http::read(socket, buffer, req, ec);
    if (ec == http::error::end_of_stream) break;
    if (ec) return fail(ec, "read");

    handle_request(std::move(req), lambda);
    if (ec) return fail(ec, "write");
    if (close) break;
  }
  socket.shutdown(tcp::socket::shutdown_send, ec);
}

int main(int argc, char* argv[]) {
  try {
    // Check command line arguments.
    if (argc != 4) {
      std::cerr << "Usage: http-server-sync <address> <port> <doc_root>\n"
                << "Example:\n"
                << "    http-server-sync 0.0.0.0 8080 .\n";
      return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(atoi(argv[2]));
    auto const doc_root = std::make_shared<std::string>(argv[3]);

    net::io_context ioc{1};
    tcp::acceptor acceptor{ioc, {address, port}};
    for (;;) {
      tcp::socket socket{ioc};
      acceptor.accept(socket);
      std::thread{bind(&do_session, std::move(socket))}.detach();
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
