#include <jde/app/AppQL.h>
#include <jde/app/log/LogQLAwait.h>
#include <jde/app/IApp.h>

namespace Jde::App{
	α AppQL::LogQuery( QL::TableQL&& ql, SL sl )ι->up<TAwait<jvalue>>{
		return mu<LogQLAwait>( move(ql), sl );
	}
	α AppQL::StatusQuery( QL::TableQL&& )ι->jobject{
		return App::IApp::Status();
	}
}