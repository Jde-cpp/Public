#pragma once
#include <boost/unordered/concurrent_flat_set.hpp>

namespace Jde::App{
	using boost::concurrent_flat_map;
	using boost::concurrent_flat_set;

	using AppPK=uint32;
	using AppInstancePK=uint32;
	using Hash=uint32;
	using LogPK=uint32;
	using RequestId=uint32;
	using StringPK=Hash;
	using ThreadPK=uint;
}
namespace Jde::App::Client{
	namespace net = boost::asio;
	namespace ssl = boost::asio::ssl;
	namespace beast = boost::beast;
	using tcp = boost::asio::ip::tcp;
}