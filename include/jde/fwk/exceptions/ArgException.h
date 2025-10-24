#include "Exception.h"
#include "ExternalException.h"

#define $ template<class... Args>
namespace Jde{
	struct ExceptionParams{
		SL _sl;
		ELogTags Tags{ ELogTags::Exception };
		ELogLevel Level{IException::DefaultLogLevel};
	};
#define Argε(x, ...) ArgException( {SRCE_CUR}, x __VA_OPT__(,) __VA_ARGS__  )
	struct Γ ArgException /*final*/ : IException{
		$ ArgException( ExceptionParams params, fmt::format_string<Args...> m, Args&& ...args )ι:
			IException{ params.Tags, params._sl, params.Level, move(m), FWD(args)... }{}

		α Move()ι->up<IException> override{ return mu<ArgException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	};
}
