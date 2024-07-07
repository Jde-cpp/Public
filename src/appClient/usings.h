#pragma once

namespace Jde::App::Client{
	namespace net = boost::asio;
	namespace ssl = boost::asio::ssl;
	namespace beast = boost::beast;
	using boost::concurrent_flat_map;
	using tcp = boost::asio::ip::tcp;

	using RequestId=uint32;
}