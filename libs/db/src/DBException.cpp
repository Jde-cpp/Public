#include <jde/db/DBException.h>
#include <jde/db/Value.h>
#include <jde/db/generators/Sql.h>
#include "DBLog.h"
#include "jde/fwk/log/logTags.h"
#include "jde/fwk/usings.h"


#define let const auto

namespace Jde::DB{
	constexpr array<sv,12> EDbErrorStrings = { "none", "duplicate", "foreignKey", "notNull", "check", "constraint", "deadlock", "timeout", "connection", "permission", "syntax", "app" }; //EDbError order.
	α ToString( EDbError x )ι->string{ return FromEnum( EDbErrorStrings, x ); }

	DBException::DBException( int32 errorCode, DB::Sql&& sql, string what, SL sl )ι:
		DBException{ EDbError::None, move(sql), move(what), {ELogLevel::Error, ELogTags::Sql, (uint32)errorCode}, sl }
	{}

	DBException::DBException( EDbError error, DB::Sql&& sql, string what, ExceptionArgs args, SL sl )ι:
		ExternalException{
			ExternalException::ECodeBase::Decimal,
			string{what},
			sql.Text,
			ExceptionArgs{ELogLevel::NoLog, args.Tags==ELogTags::None ? ELogTags::Sql : args.Tags, args._code},
			sl
		}, //NoLog here so BreakLog below dispatches to DBException::Log, which keeps the Sql context.
		Sql{ move(sql) },
		Error{ error },
		_message{ move(what) }{
		SetLevel( args.Level() ); //drivers raise connect failures as Critical; everything else defaults to Error.
		BreakLog();
	}

	α DBException::Log()Ι->void{
		if( _logged || Level()==ELogLevel::NoLog || Process::Finalizing() ) //participate in the base _logged protocol: don't re-log a moved-from/BreakLog'd exception, and stay quiet during teardown.
			return;
		_logged = true;
		DB::Log( Sql, Level(), _inner ? string{_inner->what()} : what(), _sl );
	}
}