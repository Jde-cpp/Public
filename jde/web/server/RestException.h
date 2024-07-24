#pragma once
#include "HttpRequest.h"

namespace Jde::Web::Server{
	namespace beast = boost::beast;
	namespace http = beast::http;

	struct IRestException : Exception{
	protected:
		IRestException( std::exception&& inner, HttpRequest&& req, SL sl )ι:Exception{ sl, move(inner) },_request{move(req)}{}

		template<class... Args> IRestException( SL sl, HttpRequest&& req, fmt::format_string<Args...> fmt, Args&&... args )ι:
			Exception( sl, ELogLevel::Debug, fmt, std::forward<Args>(args)... ),
			_request{ move(req) }
		{}
		template<class... Args> IRestException( SL sl, HttpRequest&& req, std::exception&& inner, fmt::format_string<Args...> fmt={}, Args&&... args )ι:
			Exception{sl, move(inner), fmt, std::forward<Args>(args)...},
			_request{ move(req) }
		{}
	public:
		constexpr β Status()Ι->http::status=0;
		α Response()Ι->http::response<http::string_body>;
		α Request()ι->HttpRequest&{ return _request; }
	private:
		string _clientMessage;
		HttpRequest _request;
	};

	template<http::status TStatus=http::status::internal_server_error>
	struct RestException final: IRestException{
		RestException( std::exception&& inner, HttpRequest&& req, SRCE )ι:IRestException{ move(inner), move(req), sl}{}
		template<class... Args> RestException( SL sl, HttpRequest&& req, fmt::format_string<Args...> fmt, Args&&... args )ι:IRestException( sl, move(req), fmt, std::forward<Args>(args)... ){}
		template<class... Args> RestException( SL sl, HttpRequest&& req, std::exception&& inner, fmt::format_string<Args...> fmt, Args&&... args )ι:IRestException{sl, move(req), move(inner), fmt, std::forward<Args>(args)...}{}
		constexpr α Status()Ι->http::status{ return TStatus; }
		α Move()ι->up<IException> override{ return mu<RestException<TStatus>>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	};

	Ξ IRestException::Response()Ι->http::response<http::string_body>{
		auto res = _request.Response<http::string_body>( Status() );
		res.body() = what();
		res.prepare_payload();
		auto& sl = Stack().front();
		auto _logTag = WebTag();
		TRACESL( "InternalServerError={}", res.body() );
		return res;
	}
}