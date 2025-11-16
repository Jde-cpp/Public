#pragma once
#ifndef OPC_UACLIENT_EXCEPTION_H
#define OPC_UACLIENT_EXCEPTION_H
#include <jde/web/server/RestException.h>
#include <jde/opc/UAException.h>
#include "../usings.h"

#define UACε(x) if( let sc = x; sc ) throw UAClientException{ sc, _ptr, requestId, ELogLevel::Debug };
namespace Jde::Web::Server{ struct HttpRequest; }
namespace Jde::Opc::Gateway{
	struct UAClientException : UAException{
		UAClientException( StatusCode sc, Handle uaHandle, RequestId requestId=0, SRCE, ELogLevel level=ELogLevel::Debug )ι:
			UAException{ sc, Ƒ("[{:x}.{:x}]{}", uaHandle, requestId, UAException::Message(sc)), sl, {level} }{}
		UAClientException( StatusCode sc, Handle uaHandle, string /*description*/, SRCE )ι:
			UAException{ sc, Ƒ("[{:x}]{}", uaHandle, UAException::Message(sc)), sl }{}
		UAClientException( UAClientException&& from )ι:UAException{ move(from) }{}
		UAClientException( const UAClientException& from )ι:UAException{ from }{}

		α Move()ι->up<IException> override{ return mu<UAClientException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
		α IsBadSession()Ι->bool{ return Code==UA_STATUSCODE_BADSESSIONIDINVALID; }
		[[noreturn]] α ThrowRest( UAClientException&& e, Web::Server::HttpRequest&& request )ε->void;
	};

	Ξ UAClientException::ThrowRest( UAClientException&& e, Web::Server::HttpRequest&& request )ε->void{
		if( e.Code==UA_STATUSCODE_BADIDENTITYTOKENREJECTED ){
			throw Web::Server::RestException<http::status::unauthorized>( move(e), move(request), "Bad identity token" );
		}
		else if( e.Code==UA_STATUSCODE_BADCONNECTIONREJECTED )
			throw Web::Server::RestException<http::status::bad_gateway>{ move(e), move(request), "Opc Server not reachable" };
		throw Web::Server::RestException<>( move(e), move(request) );
	}
}
#endif