#pragma once
#include "../usings.h"
#include "../exports.h"

namespace Jde::Web::Client{
	struct ClientHttpSession;
	struct ΓWC ClientHttpException : CodeException{
		ClientHttpException()=default;
		ClientHttpException( beast::error_code ec, ELogTags tags=ELogTags::HttpClientWrite, ELogLevel level=ELogLevel::Debug, SRCE )ι;
		ClientHttpException( beast::error_code ec, str host, PortType port={}, ELogLevel level=ELogLevel::Debug, SRCE )ι;
		ClientHttpException( beast::error_code ec, sp<ClientHttpSession>& session, http::request<http::string_body>* _req=nullptr, ELogLevel level=ELogLevel::Debug, SRCE )ι;
		ClientHttpException( ClientHttpException&& e )ι:CodeException{ move(e) }, Host{ e.Host }, Target{ e.Target }, Port{ e.Port }{}

		α SslStreamTruncated()ι{ return _errorCode.category()==ssl::error::get_stream_category() && _errorCode.value()==ssl::error::stream_truncated; }
		α Move()ι->up<IException> override{ return mu<ClientHttpException>(move(*this)); }
		α Ptr()ι->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
		const string Host;
		const string Target;
		const PortType Port{};
	};

	struct ErrorCodeException : beast::error_code{
		α Test( ELogTags tags=ELogTags::HttpClientWrite, ELogLevel l=ELogLevel::Debug, SRCE )ε->void{
			if( *this ){
				Tags=tags; Level=l; Source=sl;
				throw ClientHttpException{ *this, Tags, Level, Source };
			}
		}
		ELogTags Tags;
		ELogLevel Level;
		source_location Source;
	};
}