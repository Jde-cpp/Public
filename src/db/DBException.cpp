#include <jde/db/DBException.h>
#include <jde/db/Database.h>

namespace Jde::DB{
	α CopyParams( const std::vector<object>* pValues )ι->const std::vector<object>{
		std::vector<object> y;
		if( pValues ){
			for( uint i=0; i<pValues->size(); ++i )
				y.push_back( EObject::StringView==(EObject)(*pValues)[i].index() ? object{ string{ get<sv>((*pValues)[i])} } : (*pValues)[i] );
		}
		return y;
	}

	DBException::DBException( int32 errorCode, string sql, const std::vector<object>* pValues, string what, SL sl )ι:
		IException{ move(what), ELogLevel::NoLog, (uint)errorCode, {}, sl },
		Sql{ sql },
		Parameters{ CopyParams(pValues) }{
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