#pragma once

#define UAε(x) if( var sc = x; sc ) throw UAException{ sc, _ptr, requestId, ELogLevel::Debug };

namespace Jde::Iot{
	struct UAException : Exception{
		UAException( StatusCode sc, ELogLevel level=ELogLevel::Debug, SRCE )ι:Exception{ Message(sc), sc, level, sl }{}
		UAException( StatusCode sc, UA_Client* ua, RequestId requestId, ELogLevel level=ELogLevel::Debug, SRCE )ι:
			Exception{ Message(sc, ua, requestId), sc, level, sl }{}
		UAException( const UAException& e )ι:Exception{e.what(), e.Code, e.Level(), e.Stack().front()}{}
		UAException( UAException&& e )ι:Exception{move(e)}{}
		Ω Message( StatusCode x )ι->const char*{ return UA_StatusCode_name(x); }
		Ω Message( StatusCode x, UA_Client* ua, RequestId requestId )ι->string{ return format("[{:x}.{}] - {}", (uint)ua, requestId, UA_StatusCode_name(x)); }

		α Clone()ι->sp<IException> override{ return ms<UAException>(move(*this)); }
		α Move()ι->up<IException> override{ return mu<UAException>(move(*this)); }
		α Ptr()ι->std::exception_ptr override{ return make_exception_ptr(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }

		α ClientMessage()Ι->string{ return format( "[{:x}]{}", Code, Message((StatusCode)Code) ); }
		α IsBadSession()Ι->bool{ return Code==UA_STATUSCODE_BADSESSIONIDINVALID;}
	};
}