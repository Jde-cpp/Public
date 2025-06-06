#include "DBLog.h"
#include <jde/db/Value.h>
#include <jde/db/generators/Sql.h>

#define let const auto
namespace Jde{
	α DB::LogDisplay( const Sql& sql, string error )ι->string{
		std::ostringstream os;
		if( error.size() )
			os << move(error) << std::endl;
		uint prevIndex=0;
		auto& params = sql.Params;
		for( uint sqlIndex=0, paramIndex=0; (sqlIndex=sql.Text.find_first_of('?', prevIndex))!=string::npos && paramIndex<params.size(); ++paramIndex, prevIndex=sqlIndex+1 ){
			os << sql.Text.substr( prevIndex, sqlIndex-prevIndex );
			let& o = params[paramIndex];
			if( o.is_string() )
				os << "'";
			os << o.ToString();
			if( o.is_string() )
				os << "'";
		}
		if( prevIndex<sql.Text.size() )
			os << sql.Text.substr( prevIndex );
		return os.str();
	}
	α DB::Log( const Sql& sql, SL sl )ι->void{
		Trace{ sl, ELogTags::Sql, "{}", LogDisplay(sql, {}) };
	}
	α DB::Log( const Sql& sql, ELogLevel level, string error, SL sl )ι->void{
		Log( level, ELogTags::Sql, sl, "{}", LogDisplay(sql, move(error)) );
	}
	α DB::LogNoServer( const Sql& sql, ELogLevel level, string error, SL sl )ι->void{
		Log( level, ELogTags::Sql | ELogTags::ExternalLogger, sl, "{}", LogDisplay(sql, move(error)) );
	}
}
