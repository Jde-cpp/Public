#pragma once
#include "../flex/HttpRequest.h"

namespace Jde::Web::Rest{
	namespace beast = boost::beast;
	namespace http = beast::http;

	struct IRestException : Exception{
		IRestException()ι:Exception{""}{}
		template<class... Args> IRestException( SL sl, fmt::format_string<Args...> fmt, Args&&... args )ι:Exception( sl, ELogLevel::Debug, fmt, std::forward<Args>(args)... ){}
		template<class... Args> IRestException( SL sl, std::exception&& inner, fmt::format_string<Args...> fmt={}, Args&&... args )ι:Exception{sl, move(inner), fmt, std::forward<Args>(args)...}{}
		consteval β Status()Ι->http::status=0;
		α Response( const Flex::HttpRequest&& req )Ι->http::response<http::string_body>;
	private:
		string _clientMessage;
	};

	template<http::status TStatus=http::status::internal_server_error>
	struct RestException : IRestException{
		RestException()ι:IRestException{}{}
		template<class... Args> RestException( SL sl, fmt::format_string<Args...> fmt, Args&&... args )ι:IRestException( sl, fmt, std::forward<Args>(args)... ){}
		template<class... Args> RestException( SL sl, std::exception&& inner, fmt::format_string<Args...> fmt={}, Args&&... args )ι:IRestException{sl, move(inner), fmt, std::forward<Args>(args)...}{}
		consteval α Status()Ι->http::status{ return TStatus; }
	};

	Ξ IRestException::Response( const Flex::HttpRequest&& req )Ι->http::response<http::string_body>{
		auto res = req.Response<http::string_body>( Status() );
		res.body() = what();
		res.prepare_payload();
		auto& sl = Stack().front();
		auto _logTag = Flex::WebTag();
		TRACESL( "InternalServerError={}", res.body() );
		return res;
	}
}