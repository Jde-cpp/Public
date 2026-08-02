#pragma once
#include <jde/db/DBException.h>

namespace Jde::App::ProtoUtils{
	α ToException( Exception&& e )ι->Jde::Proto::Exception;
	//up<>, not by value: the web server branches on catch( DB::DBException& ), and a Jde::Exception return slices the classification back off.
	Ξ ToException( Jde::Proto::Exception&& e, SRCE )ι->up<Jde::Exception>{
		return e.db_error()
			? up<Jde::Exception>{ mu<DB::DBException>( (DB::EDbError)e.db_error(), DB::Sql{}, e.what(), ExceptionArgs{e.code()}, sl ) }
			: mu<Jde::Exception>( e.what(), ExceptionArgs{e.code()}, sl );
	}
	α ToQuery( string&& text, jobject&& variables, bool returnRaw )ι->Jde::Proto::Query;
}
