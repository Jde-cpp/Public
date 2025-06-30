#include <jde/db/DBException.h>
#include <jde/db/Value.h>
#include <jde/db/generators/Sql.h>
#include "DBLog.h"


#define let const auto

namespace Jde::DB{
	DBException::DBException( int32 errorCode, DB::Sql&& sql, string what, SL sl )ι:
		IException{ move(what), ELogLevel::NoLog, (uint32)errorCode, {}, sl },
		Sql{ move(sql) }{
		SetLevel( ELogLevel::Error );
		BreakLog();
	}

	α DBException::what()const noexcept->const char*{
		if( _what.empty() && Sql.Text.size() )
			_what = DB::LogDisplay( Sql, IException::what() );
		return IException::what();
	}

	α DBException::Log()Ι->void{
		if( Level()==ELogLevel::NoLog )
			return;
		if( Sql.Text.find("log_message_insert")==string::npos && Sql.Text.find("log_files")==string::npos && Sql.Text.find("log_functions")==string::npos )
			DB::Log( Sql, Level(), _pInner ? string{_pInner->what()} : what(), _stack.front() );
		else
			DB::LogNoServer( Sql, ELogLevel::Error, what(), _stack.front() );
	}
}