#pragma once
#include <jde/fwk/str.h>
#include "Exception.h"

namespace Jde{
	struct ExternalException : Exception{
		ExternalException( const ExternalException& from )ι:
			Exception{ string{from.Format()}, {ELogLevel::NoLog, from.Tags, from._code}, from._sl }{
			_args = from._args;
		}
		ExternalException( ExternalException&& from )ι=default;

		ExternalException( string externalMessage, string description, ExceptionArgs args, SRCE )ι:
			Exception{ FormatMsg(move(externalMessage), description.size(), args), {ELogLevel::NoLog, args.Tags, args._code}, sl }{
			if( description.size() ){
				_what = Str::Format( Format(), {description} );
				_args.push_back( move(description) );
			}
			SetLevel( args.Level() );
			BreakLog();
		}
	private:
		//error code is part of format; ' - {}' description suffix only when a description exists.
		Ω FormatMsg( string&& externalMessage, bool hasDescription, const ExceptionArgs& args )ι->string{
			const sv suffix = hasDescription ? " - {}" : "";
			return args.HasCode() ? Ƒ("({:x}){}{}", args.Code(), externalMessage, suffix) : Ƒ("{}{}", externalMessage, suffix);
		}
	};
}
