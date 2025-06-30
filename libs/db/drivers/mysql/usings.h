#pragma once
namespace Jde::DB::MySql{
	constexpr ELogTags _tags{ ELogTags::Sql };
	namespace mysql = boost::mysql;
	namespace asio = boost::asio;
}