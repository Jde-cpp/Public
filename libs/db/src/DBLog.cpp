#include "DBLog.h"
#include <jde/db/Value.h>

#define let const auto
namespace Jde{
	α DB::LogDisplay( sv sql, const vector<Value>* pParameters, string error )ι->string{
		std::ostringstream os;
		if( error.size() )
			os << move(error) << std::endl;
		uint prevIndex=0;
		for( uint sqlIndex=0, paramIndex=0, size = pParameters ? pParameters->size() : 0; (sqlIndex=sql.find_first_of('?', prevIndex))!=string::npos && paramIndex<size; ++paramIndex, prevIndex=sqlIndex+1 ){
			os << sql.substr( prevIndex, sqlIndex-prevIndex );
			let& o = (*pParameters)[paramIndex];
			if( o.is_string() )
				os << "'";
			os << o.ToString();
			if( o.is_string() )
				os << "'";
		}
		if( prevIndex<sql.size() )
			os << sql.substr( prevIndex );
		return os.str();
	}
	α DB::Log( sv sql, const vector<Value>* pParameters, SL sl )ι->void{
		Trace{ sl, ELogTags::Sql, "{}", LogDisplay(sql, pParameters, {}) };
	}
	α DB::Log( sv sql, const vector<Value>* pParameters, ELogLevel level, string error, SL sl )ι->void{
		Log( level, ELogTags::Sql, sl, "{}", LogDisplay(sql, pParameters, error) );
	}
	α DB::LogNoServer( string sql, const vector<Value>* pParameters, ELogLevel level, string error, SL sl )ι->void{
		Log( level, ELogTags::Sql | ELogTags::ExternalLogger, sl, "{}", LogDisplay(sql, pParameters, error) );
	}
}
