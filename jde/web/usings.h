#pragma once
#include <jde/TypeDefs.h>
#include <jde/Exports.h>
#include "../../../Framework/source/io/ServerSink.h"
#include "exports.h"

namespace Jde::Web{
	using SessionPK=uint32;
	using SessionInfo=Logging::Proto::SessionInfo;
	namespace Flex{ 
		namespace net = boost::asio;
		namespace beast = boost::beast;
		namespace http = beast::http;
		namespace websocket = beast::websocket;
		namespace ssl = boost::asio::ssl;
		using tcp = boost::asio::ip::tcp;
		using executor_type = net::io_context::executor_type;	
		using executor_with_default = net::as_tuple_t<net::use_awaitable_t<executor_type>>::executor_with_default<executor_type>;
	}
}