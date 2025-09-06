#pragma once
#ifndef EXTERNAL_LOGGER_H
#define EXTERNAL_LOGGER_H
#include "Entry.h"
#include "logTags.h"

#define Φ Γ auto
#define FormatString const fmt::format_string<Args const&...>
#define ARGS const Args&
namespace Jde::Logging{
	struct Γ ILogger : LogTags{
		ILogger( jobject o ): LogTags( o ){}
		virtual ~ILogger(){} //important
		β Name()ι->string=0;
		β Write( const Entry& m )ι->void=0;
	};
}
#undef Φ
#undef FormatString
#undef ARGS
#endif