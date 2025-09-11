#pragma once
#include <jde/framework/str.h>

namespace Jde{
	struct ExternalException : IException{
		//when error code is part of format.
		ExternalException( string externalMessage, string description, SL sl, ELogTags tags=ELogTags::None, ELogLevel l=IException::DefaultLogLevel ):
			IException{ {}, ELogLevel::NoLog, 0, tags, sl },
			_errorFormat{ Ƒ("{} - {{}}", externalMessage) }{
			Initialize( move(description), l );
		}

		ExternalException( string externalMessage, string description, uint code, SL sl, ELogTags tags, ELogLevel l=IException::DefaultLogLevel )ι:
			IException{ {}, ELogLevel::NoLog, 0, tags, sl },
			_errorFormat{ Ƒ("({:x}){} - {{}}", code, externalMessage) }{
			Initialize( move(description), l );
		}

	private:
		α Initialize( string&& description, ELogLevel level )->void{
			_format = _errorFormat;
			_what = Str::Format( _format, {description} );
			_args.push_back( move(description) );
			SetLevel( level );
			BreakLog();
		}
		string _errorFormat;
	};
}
