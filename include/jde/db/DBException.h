#pragma once
#include "exports.h"
#include "Value.h"

#define Φ ΓDB auto
namespace Jde::DB{
	struct Value;

	struct ΓDB DBException final: IException{
		DBException( int32 errorCode, string sql, const vector<Value>* pValues, string what, SRCE )ι;
		DBException( string sql, const vector<Value>* pValues, string what, SRCE )ι:DBException{ 0, move(sql), pValues, move(what), sl }{}
		DBException( DBException&& from )ι:IException{move(from)}, Sql{move(from.Sql)}, Parameters{move(from.Parameters)}{}
		DBException( const DBException& from )ι=default;
		~DBException(){ Log(); SetLevel( ELogLevel::NoLog ); };

		α Log()Ι->void override;
		α what()const noexcept->const char* override;
		using T=DBException;
		α Move()ι->up<IException> override{ return mu<T>(move(*this)); }
		α Ptr()ι->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }
		[[noreturn]] α Throw()ε->void override{ throw move(*this); }

		const string Sql;
		const vector<Value> Parameters;
	};
}
#undef Φ