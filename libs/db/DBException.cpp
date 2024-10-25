#include <jde/db/DBException.h>
#include <jde/db/Value.h>
#include "DBLog.h"


#define let const auto

namespace Jde::DB{
	DBException::DBException( int32 errorCode, string sql, const vector<Value>* pValues, string what, SL sl )ι:
		IException{ move(what), ELogLevel::NoLog, (uint)errorCode, {}, sl },
		Sql{ sql },
		Parameters{ pValues ? *pValues : vector<Value>{} }{
		SetLevel( ELogLevel::Error );
		BreakLog();
	}

	α DBException::what()const noexcept->const char*{
		if( Sql.size() && _what.empty() )
			_what =  DB::LogDisplay( Sql, &Parameters, IException::what() );
		return IException::what();
	}

	α DBException::Log()Ι->void{
		if( Level()==ELogLevel::NoLog )
			return;
		if( Sql.find("log_message_insert")==string::npos && Sql.find("log_files")==string::npos && Sql.find("log_functions")==string::npos )
			DB::Log( Sql, Parameters.size() ? &Parameters : nullptr, Level(), _pInner ? string{_pInner->what()} : what(), _stack.front() );
		else
			DB::LogNoServer( Sql, &Parameters, ELogLevel::Error, what(), _stack.front() );
	}
}