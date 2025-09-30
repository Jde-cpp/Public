#pragma once
#include <jde/fwk/str.h>

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
		ExternalException( ExternalException&& from )ι:IException{ move(from) }{}
		ExternalException( const ExternalException& from )ι:
			IException{ get<string>(from._format), ELogLevel::NoLog, 0, from._tags, from._stack.stack.front() }{
			_args = from._args;
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
