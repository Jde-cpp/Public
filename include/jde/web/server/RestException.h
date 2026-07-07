#pragma once
#include "HttpRequest.h"
#include "jde/fwk.h"

namespace Jde::Web::Server{
	struct IRestException : Exception{
	protected:
		IRestException( Exception&& inner, HttpRequest&& req )ι:Exception{ move(inner) },_request{move(req)}{}
		IRestException( const IRestException& rhs );
		IRestException( IRestException&& ) = default;

		template<class... Args> IRestException( SL sl, HttpRequest&& req, fmt::format_string<Args...> fmt, Args&&... args )ι:
			Exception( sl, {}, fmt, FWD(args)... ),
			_request{ move(req) }
		{}
		template<class... Args> IRestException( Exception&& inner, HttpRequest&& req, fmt::format_string<Args...> fmt, Args&&... args )ι:
			Exception{ inner.Source(), inner, move(inner), fmt, FWD(args)... },
			_request{ move(req) }
		{}
	public:
		constexpr β Status()Ι->http::status=0;
		α Response()Ι->http::response<http::string_body>;
		α Request()ι->HttpRequest&{ return _request; }
		//α Ptr()ι->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }
	private:
		HttpRequest _request;
	};

	template<http::status TStatus=http::status::internal_server_error>
	struct RestException final: IRestException{
		RestException( Exception&& inner, HttpRequest&& req )ι:IRestException{ move(inner), move(req)}{}
		template<class... Args> RestException( SL sl, HttpRequest&& req, fmt::format_string<Args...> fmt, Args&&... args )ι:IRestException( sl, move(req), fmt, FWD(args)... ){}
		template<class... Args> RestException( Exception&& inner, HttpRequest&& req, fmt::format_string<Args...> fmt, Args&&... args )ι:IRestException{ move(inner), move(req), fmt, FWD(args)...}{}
		RestException( const RestException& rhs ):IRestException{rhs}{ ASSERT(false); }
		RestException( RestException&& ) = default;
		constexpr α Status()Ι->http::status override{ return TStatus; }
		α Move()ι->up<Exception> override{ return mu<RestException<TStatus>>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	};

	inline IRestException::IRestException( const IRestException& rhs ):
		Exception{rhs},
		_request{TRequestType{rhs._request._request}, rhs._request.UserEndpoint, rhs._request._isSsl, rhs._request._connectionId }{
		ASSERT(false);
	}
	Ξ IRestException::Response()Ι->http::response<http::string_body>{
		auto res = _request.Response<http::string_body>( Status() );
		res.body() = what();
		res.prepare_payload();
		return res;
	}
}