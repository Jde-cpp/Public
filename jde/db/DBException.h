#pragma once
#include <jde/Exports.h>
#include <jde/db/DataType.h>
#include <jde/Exception.h>

#define DB_TRY(x) try{x;}catch( const DB::DBException& ){}
#define THROW_DB(sql,params)
namespace Jde::DB
{
	struct Γ DBException final: IException
	{
		DBException( int32 errorCode, string sql, const vector<object>* pValues, string what, SRCE )ι;
		DBException( string sql, const vector<object>* pValues, string what, SRCE )ι:DBException{ 0, move(sql), pValues, move(what), sl }{}
		DBException( DBException&& from )ι:IException{move(from)}, Sql{from.Sql}, Parameters{from.Parameters}{}
		DBException( const DBException& from )ι=default;
		~DBException(){ Log(); SetLevel( ELogLevel::NoLog ); };

		α Log()Ι->void override;
		α what()const noexcept->const char* override;
		using T=DBException;
		α Move()ι->up<IException> override{ return mu<T>(move(*this)); }
		α Ptr()ι->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }
		[[noreturn]] α Throw()ε->void override{ throw move(*this); }

		const string Sql;
		const vector<object> Parameters;
	};
}