#pragma once
#include "../usings.h"
#include <boost/beast/ssl/ssl_stream.hpp>

namespace Jde::Web::Client{
	using SslSocketStream =	websocket::stream<beast::ssl_stream<beast::tcp_stream>>;
}