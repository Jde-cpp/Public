#pragma once
#include "exports.h"
#include "Value.h"

#define Φ ΓDB auto
namespace Jde::DB{
	struct Sql; struct Value;

	struct ΓDB DBException final: IException{
		DBException( int32 errorCode, Sql&& sql, string what, SRCE )ι;
		DBException( Sql&& sql, string what, SRCE )ι:DBException{ 0, move(sql), move(what), sl }{}
		DBException( DBException&& from )ι:IException{move(from)}, Sql{move(from.Sql)}{}
		DBException( const DBException& from )ι=default;
		~DBException(){ Log(); SetLevel( ELogLevel::NoLog ); };

		α Log()Ι->void override;
		α what()const noexcept->const char* override;
		using T=DBException;
		α Move()ι->up<IException> override{ return mu<T>(move(*this)); }
		α Ptr()ι->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }
		[[noreturn]] α Throw()ε->void override{ throw move(*this); }

		DB::Sql&& Sql;
	};
}
#undef Φ