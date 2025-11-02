#pragma once
#include "Exception.h"
#include "ExternalException.h"

namespace Jde{
	struct Γ CodeException /*final*/ : ExternalException{
		CodeException( std::error_code code, ELogTags tags, ELogLevel level=ELogLevel::Debug, SRCE )ι;
		CodeException( std::error_code code, ELogTags tags, string value, ELogLevel level=ELogLevel::Debug, SRCE )ι;

		 α Move()ι->up<IException> override{ return mu<CodeException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }

		Ω ToString( const std::error_code& pErrorCode )ι->string;
		Ω ToString( const std::error_category& errorCategory )ι->string;
		Ω ToString( const std::error_condition& errorCondition )ι->string;
	protected:
		std::error_code _errorCode;
	};
}