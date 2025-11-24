#pragma once
#include <boost/beast.hpp>

namespace Jde{
	using RequestId = uint32;
	namespace net = boost::asio;
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace websocket = beast::websocket;
	namespace ssl = net::ssl;
	using tcp = net::ip::tcp;
}
