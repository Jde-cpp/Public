#include "DBLog.h"
#include <jde/db/Value.h>
#include <jde/db/generators/Sql.h>

#define let const auto
namespace Jde{
	constexpr ELogTags _tags{ ELogTags::Sql };
	α DB::LogDisplay( const Sql& sql, string error )ι->string{
		string result = move( error );
		if( result.size() )
			result += "\n";
		result += sql.EmbedParams();
		return result;
	}
	α DB::Log( const Sql& sql, SL sl )ι->void{
		TRACESL( "sql: {}", sql.EmbedParams() );
	}
	α DB::Log( const Sql& sql, ELogLevel level, string error, SL sl )ι->void{
		Logging::Log( level, _tags, sl, "sqle: {}", LogDisplay(sql, move(error)) );
	}
	α DB::LogNoServer( const Sql& sql, ELogLevel level, string error, SL sl )ι->void{
		Logging::Log( level, _tags | ELogTags::ExternalLogger, sl, "sqle: {}", LogDisplay(sql, move(error)) );
	}
}
