#pragma once
#include <jde/framework/str.h>

namespace Jde{
	struct ExternalException : IException{
		//when error code is part of format.
		ExternalException( string externalMessage, string description, SL sl, ELogTags tags=ELogTags::None, ELogLevel l=IException::DefaultLogLevel ):
			IException{ Ƒ("{} - {{}}", externalMessage), ELogLevel::NoLog, 0, tags, sl }{
			Initialize( move(description), l );
		}

		ExternalException( string externalMessage, string description, uint code, SL sl, ELogTags tags, ELogLevel l=IException::DefaultLogLevel )ι:
			IException{ Ƒ("({:x}){} - {{}}", code, externalMessage), ELogLevel::NoLog, 0, tags, sl }{
			Initialize( move(description), l );
		}

	private:
		α Initialize( string&& description, ELogLevel level )->void{
			_what = Str::Format( Format(), {description} );
			_args.push_back( move(description) );
			SetLevel( level );
			BreakLog();
		}
	};
}
