#pragma once
#ifndef LOGGER_H
#define LOGGER_H
#include "logTags.h"

namespace Jde::Logging{
	struct Entry;
	struct Γ ILogger : LogTags, IShutdown{
		ILogger( jobject o ): LogTags( move(o) ){}
		virtual ~ILogger(){} //important
		β Write( const Entry& m )ι->void=0;
	};
}
#endif