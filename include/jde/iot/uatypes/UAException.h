#pragma once

#define UAε(x) if( let sc = x; sc ) throw UAException{ sc, _ptr, requestId, ELogLevel::Debug };

namespace Jde::Iot{
	struct UAException : IException{
		UAException( StatusCode sc, ELogLevel level=ELogLevel::Debug, SRCE )ι:IException{ Message(sc), level, sc, {}, sl }{}
		UAException( StatusCode sc, UA_Client* ua, RequestId requestId, ELogLevel level=ELogLevel::Debug, SRCE )ι:
			IException{ Message(sc, ua, requestId), level, sc, {}, sl }{}
		UAException( const UAException& e )ι:IException{e.what(), e.Level(), e.Code, {}, e.Stack().front()}{} //copy for multiple requests.
		UAException( UAException&& e )ι:IException{move(e)}{}//so copy constructor is not called.
		Ω Message( StatusCode x )ι->const char*{ return UA_StatusCode_name(x); }
		Ω Message( StatusCode x, UA_Client* ua, RequestId requestId )ι->string{ return format("[{:x}.{}] - {}", (uint)ua, requestId, UA_StatusCode_name(x)); }

		α Move()ι->up<IException> override{ return mu<UAException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }

		α ClientMessage()Ι->string{ return format( "[{:x}]{}", Code, Message((StatusCode)Code) ); }
		α IsBadSession()Ι->bool{ return Code==UA_STATUSCODE_BADSESSIONIDINVALID;}
	};
}