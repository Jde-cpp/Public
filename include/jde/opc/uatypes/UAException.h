#pragma once
#ifndef OPC_UAEXCEPTION_H
#define OPC_UAEXCEPTION_H
#include "../usings.h"

#define UAε(x) if( let sc = x; sc ) throw UAException{ sc };

namespace Jde::Opc{
	struct UAException : IException{
		UAException( StatusCode sc, ELogLevel level=ELogLevel::Debug, SRCE )ι:IException{ Message(sc), level, sc, {}, sl }{}
		UAException( string&& what, ELogLevel level, StatusCode sc, SL sl ):IException{ Ƒ("{} - {}", Message(sc), move(what)), level, sc, {}, sl }{}
		Ω Message( StatusCode x )ι->const char*{ return UA_StatusCode_name(x); }

		α Move()ι->up<IException> override{ return mu<UAException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }

		α ClientMessage()Ι->string{ return format( "({:x}){}", Code, Message((StatusCode)Code) ); }
	};
}
#endif