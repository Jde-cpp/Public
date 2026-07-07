#include <jde/db/DBException.h>
#include <jde/db/Value.h>
#include <jde/db/generators/Sql.h>
#include "DBLog.h"


#define let const auto

namespace Jde::DB{
	DBException::DBException( int32 errorCode, DB::Sql&& sql, string what, SL sl )ι:
		Exception{ move(what), {ELogLevel::NoLog, ELogTags::Exception, (uint32)errorCode}, sl },
		Sql{ move(sql) }{
		SetLevel( ELogLevel::Error );
		BreakLog(); //here rather than in args: base-ctor BreakLog would dispatch to Exception::Log, losing the Sql context.
	}

	α DBException::what()const noexcept->const char*{
		if( _what.empty() && Sql.Text.size() )
			_what = DB::LogDisplay( Sql, Exception::what() );
		return Exception::what();
	}

	α DBException::Log()Ι->void{
		if( Level()==ELogLevel::NoLog )
			return;
		if( Sql.Text.find("log_message_insert")==string::npos && Sql.Text.find("log_files")==string::npos && Sql.Text.find("log_functions")==string::npos )
			DB::Log( Sql, Level(), _inner ? string{_inner->what()} : what(), _sl );
		else
			DB::LogNoServer( Sql, ELogLevel::Error, what(), _sl );
	}
}