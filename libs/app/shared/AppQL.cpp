#include <jde/app/AppQL.h>
#include <jde/app/log/LogQLAwait.h>
#include <jde/app/IApp.h>

namespace Jde::App{
	α AppQL::RequireAuthenticated( QL::Creds& executer, sv what, SL sl )ε->void{
		if( !executer.UserPK().Value )
			throw Exception{ sl, {ELogTags::App | ELogTags::Exception, 0, EHttpStatus::Unauthorized}, "[{}]An authenticated user is required.", what };
	}
	α AppQL::LogQuery( QL::TableQL&& ql, QL::Creds executer, SL sl )ε->up<TAwait<jvalue>>{
		RequireAuthenticated( executer, "logs", sl ); //every process log line - query texts, user names, cert DNs, hostnames, session ids.
		return mu<LogQLAwait>( move(ql), sl );
	}
	α AppQL::StatusQuery( QL::TableQL&&, QL::Creds executer, SL sl )ε->jobject{
		RequireAuthenticated( executer, "status", sl );
		return App::IApp::Status();
	}
}