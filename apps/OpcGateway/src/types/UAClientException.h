#pragma once
#ifndef OPC_UACLIENT_EXCEPTION_H
#define OPC_UACLIENT_EXCEPTION_H
#include <jde/web/server/RestException.h>
#include <jde/opc/UAException.h>
#include "../usings.h"

#define UACε(f) if( let sc = f; sc ) throw UAClientException{ sc, _client, _requestId, _sl, ELogLevel::Debug };
namespace Jde::Web::Server{ struct HttpRequest; }
namespace Jde::Opc::Gateway{
	struct UAClient;
	struct UAClientException : UAException{
		UAClientException( StatusCode sc, sp<UAClient> client, RequestId requestId=0, SRCE, ELogLevel level=ELogLevel::Debug )ι;
		UAClientException( StatusCode sc, Handle uaHandle, RequestId requestId=0, SRCE, ELogLevel level=ELogLevel::Debug )ι:
			UAException{ sc, Ƒ("[{:x}.{:x}]{}", uaHandle, requestId, UAException::Message(sc)), sl, {level} }{}
		//the description is the throw-site's reason ("Connection Failed", "writeValueAttribute", an applicationUri
		//mismatch...); it was previously discarded, leaving the status name as the only clue anywhere.
		UAClientException( StatusCode sc, Handle uaHandle, string description, SRCE )ι:
			UAException{ sc, Ƒ("[{:x}]{}", uaHandle, description.size() ? sv{description} : sv{UAException::Message(sc)}), sl },
			_detail{ move(description) }{}
		UAClientException( UAClientException&& from )ι:UAException{ move(from) }, _detail{ move(from._detail) }{}
		UAClientException( const UAClientException& from )ι:UAException{ from }, _detail{ from._detail }{}

		α Move()ι->up<Exception> override{ return mu<UAClientException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
		α IsBadSession()Ι->bool{ return Code()==UA_STATUSCODE_BADSESSIONIDINVALID; }
		//status plus the throw-site reason; the description names an operation or a configuration mismatch, never internals.
		α ClientDetail()Ι->string override{ return _detail.size() ? Ƒ("{} - {}", UAException::ClientDetail(), _detail) : UAException::ClientDetail(); }
		[[noreturn]] α ThrowRest( UAClientException&& e, Web::Server::HttpRequest&& request )ε->void;
	private:
		string _detail;
	};

	Ξ UAClientException::ThrowRest( UAClientException&& e, Web::Server::HttpRequest&& request )ε->void{
		if( e.Code()==UA_STATUSCODE_BADIDENTITYTOKENREJECTED ){
			throw Web::Server::RestException<http::status::unauthorized>( move(e), move(request), "Bad identity token" );
		}
		else if( e.Code()==UA_STATUSCODE_BADCONNECTIONREJECTED )
			throw Web::Server::RestException<http::status::bad_gateway>{ move(e), move(request), "Opc Server not reachable" };
		throw Web::Server::RestException<>( move(e), move(request) );
	}
}
#endif