#pragma once
#include "jde/fwk/log/logTags.h"
#ifndef OPC_UAEXCEPTION_H
#define OPC_UAEXCEPTION_H
#include "usings.h"
#include <jde/fwk/exceptions/ExternalException.h>
#include "uatypes/Logger.h"

#define UAε(x) if( let sc = x; sc ) throw UAException{ sc };

namespace Jde::Opc{
	struct UAExParams{
		ELogLevel Level{DefaultExceptionLevel};
		EOpcLogTags Tags{EOpcLogTags::Opc};
	};
	struct UAException : ExternalException{
		//UAException( StatusCode sc, ELogLevel level=Exception::DefaultLogLevel, SRCE )ι:ExternalException{ Message(sc), {}, sc, sl, level, sl, {} }{}
		UAException( StatusCode sc, string what={}, SRCE, UAExParams params={} ):ExternalException{ Message(sc), what, {params.Level, (ELogTags)params.Tags, (uint32)sc}, sl }{}
		UAException( UAException&& from )ι:ExternalException{ move(from) }{}
		UAException( const UAException& from )ι:ExternalException{ from }{}
		Ω Message( StatusCode sc )ι->const char*{ return UA_StatusCode_name(sc); }
		Ω ToJson( StatusCode sc, bool includeMsg=false )ι->jobject{ return includeMsg ? jobject{{"sc", sc},{"message", UAException::Message(sc)}} :  jobject{{"sc", sc}}; }

		α Move()ι->up<Exception> override{ return mu<UAException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }

		α ClientMessage()Ι->string{ return format( "({:x}){}", Code(), Message((StatusCode)Code()) ); }
	};
}
#endif