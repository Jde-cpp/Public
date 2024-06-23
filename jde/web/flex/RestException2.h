#pragma once
#include "HttpRequest.h"

namespace Jde::Web::Flex{
	namespace beast = boost::beast;
	namespace http = beast::http;

	struct IRestException : Exception{
		//IRestException()ι:Exception{""}{}
		template<class... Args> IRestException( SL sl, HttpRequest&& req, fmt::format_string<Args...> fmt, Args&&... args )ι:
			Exception( sl, ELogLevel::Debug, fmt, std::forward<Args>(args)... ),
			_request{ move(req) }
		{}
		template<class... Args> IRestException( SL sl, HttpRequest&& req, std::exception&& inner, fmt::format_string<Args...> fmt={}, Args&&... args )ι:
			Exception{sl, move(inner), fmt, std::forward<Args>(args)...},
			_request{ move(req) }
		{}
		constexpr β Status()Ι->http::status=0;
		α Response()Ι->http::response<http::string_body>;
		α Request()ι->HttpRequest&{ return _request; }
	private:
		string _clientMessage;
		Flex::HttpRequest _request;
	};

	template<http::status TStatus=http::status::internal_server_error>
	struct RestException : IRestException{
		//RestException()ι:IRestException{}{}
		template<class... Args> RestException( SL sl, HttpRequest&& req, fmt::format_string<Args...> fmt, Args&&... args )ι:IRestException( sl, move(req), fmt, std::forward<Args>(args)... ){}
		template<class... Args> RestException( SL sl, HttpRequest&& req, std::exception&& inner, fmt::format_string<Args...> fmt={}, Args&&... args )ι:IRestException{sl, move(req), move(inner), fmt, std::forward<Args>(args)...}{}
		constexpr α Status()Ι->http::status{ return TStatus; }
		α Move()ι->up<IException> override{ return mu<RestException<TStatus>>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	};

	Ξ IRestException::Response()Ι->http::response<http::string_body>{
		auto res = _request.Response<http::string_body>( Status() );
		res.body() = what();
		res.prepare_payload();
		auto& sl = Stack().front();
		auto _logTag = Flex::WebTag();
		TRACESL( "InternalServerError={}", res.body() );
		return res;
	}
}