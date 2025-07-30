#pragma once
#ifndef OPC_UACLIENT_EXCEPTION_H
#define OPC_UACLIENT_EXCEPTION_H
#include <jde/opc/uatypes/UAException.h>
#include "../usings.h"

#define UACε(x) if( let sc = x; sc ) throw UAClientException{ sc, _ptr, requestId, ELogLevel::Debug };

namespace Jde::Opc::Gateway{
	struct UAClientException : UAException{
		UAClientException( StatusCode sc, ELogLevel level=ELogLevel::Debug, SRCE )ι:UAException{ sc, level, sl }{}
		UAClientException( StatusCode sc, UA_Client* ua, RequestId requestId, ELogLevel level=ELogLevel::Debug, SRCE )ι:
			UAException{ Message(sc, ua, requestId), level, sc, sl }{}
		UAClientException( const UAClientException& e )ι:UAException{e}{} //copy for multiple requests.
		UAClientException( UAClientException&& e )ι:UAException{move(e)}{}
		Ω Message( StatusCode x, UA_Client* ua, RequestId requestId )ι->string{ return format("[{:x}.{}] - {}", (uint)ua, requestId, UA_StatusCode_name(x)); }

		α Move()ι->up<IException> override{ return mu<UAClientException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
		α IsBadSession()Ι->bool{ return Code==UA_STATUSCODE_BADSESSIONIDINVALID; }
	};
}
#endif