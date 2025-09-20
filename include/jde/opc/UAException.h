#pragma once
#ifndef OPC_UAEXCEPTION_H
#define OPC_UAEXCEPTION_H
#include "usings.h"
#include <jde/framework/exceptions/ExternalException.h>
#include "uatypes/Logger.h"

#define UAε(x) if( let sc = x; sc ) throw UAException{ sc };

namespace Jde::Opc{
	struct UAExParams{
		ELogLevel Level{IException::DefaultLogLevel};
		EOpcLogTags Tags{EOpcLogTags::Opc};
	};
	struct UAException : ExternalException{
		//UAException( StatusCode sc, ELogLevel level=IException::DefaultLogLevel, SRCE )ι:ExternalException{ Message(sc), {}, sc, sl, level, sl, {} }{}
		UAException( StatusCode sc, string what={}, SRCE, UAExParams params={} ):ExternalException{ Message(sc), what, sc, sl, (ELogTags)params.Tags, params.Level }{}
		UAException( UAException&& from )ι:ExternalException{ move(from) }{}
		UAException( const UAException& from )ι:ExternalException{ from }{}
		Ω Message( StatusCode x )ι->const char*{ return UA_StatusCode_name(x); }

		α Move()ι->up<IException> override{ return mu<UAException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }

		α ClientMessage()Ι->string{ return format( "({:x}){}", Code, Message((StatusCode)Code) ); }
	};
}
#endif