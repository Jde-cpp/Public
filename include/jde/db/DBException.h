#pragma once
#include "exports.h"
#include <jde/db/generators/Sql.h>

namespace Jde::DB{
	struct ΓDB DBException final : Exception{
		DBException( int32 errorCode, DB::Sql&& sql, string what, SRCE )ι;
		DBException( DB::Sql&& sql, string what, SRCE )ι:DBException{ 0, move(sql), move(what), sl }{}
		DBException( DBException&& from )ι:Exception{move(from)}, Sql{move(from.Sql)}{}
		DBException( const DBException& from )ι=delete;
		~DBException(){ Log(); SetLevel( ELogLevel::NoLog ); };

		α Log()Ι->void override;
		α what()const noexcept->const char* override;
		α Move()ι->up<Exception> override{ return mu<DBException>(move(*this)); }
		[[noreturn]] α Throw()ε->void override{ throw move(*this); }

		DB::Sql Sql;
	};
}