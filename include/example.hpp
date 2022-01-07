// Copyright 2022 SovesT

#ifndef INCLUDE_EXAMPLE_HPP_
#define INCLUDE_EXAMPLE_HPP_

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <thread>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>
using std::string;
using std::regex;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::map;
using std::smatch;
using std::regex_search;
using std::bind;
using std::atoi;

void fail(beast::error_code, char const*);

#endif // INCLUDE_EXAMPLE_HPP_
