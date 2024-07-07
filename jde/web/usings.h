#pragma once
#include <boost/unordered/concurrent_flat_map.hpp>
#include <jde/TypeDefs.h>
#include <jde/Exports.h>
#include "exports.h"

namespace Jde::Web{
	using SessionPK=uint32;
	//using SessionInfo=Logging::Proto::SessionInfo;
	using boost::concurrent_flat_map;
	namespace Flex{
		namespace net = boost::asio;
		namespace beast = boost::beast;
		namespace http = beast::http;
		namespace websocket = beast::websocket;
		namespace ssl = boost::asio::ssl;
		using tcp = boost::asio::ip::tcp;
		using executor_type = net::io_context::executor_type;
		using executor_with_default = net::as_tuple_t<net::use_awaitable_t<executor_type>>::executor_with_default<executor_type>;

		using TBody=http::string_body;
		using TAllocator=std::allocator<char>;
		using TRequestType = http::request<TBody, http::basic_fields<TAllocator>>;

		using StreamType = typename beast::tcp_stream::rebind_executor<executor_with_default>::other;
	}
}