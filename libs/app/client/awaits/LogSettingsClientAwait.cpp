#include <jde/app/client/awaits/LogSettingsClientAwait.h>
#include <jde/app/IApp.h>
#include <jde/app/log/ProtoLog.h>
#include <jde/app/client/RemoteLog.h>

#define let const auto

namespace Jde::App::Client{
	//logSetting{ text binary appServer tags }
	α LogSettingsClientAwait::await_ready()ι->bool{
		base::await_ready();
		if( auto appServer = _ql.FindColumn("appServer"); appServer )
			_result["appServer"] = ToJson<RemoteLog>();
		return true;
	}

	//updateLogSettings( text(default: Information,settings: Warning), binary(...) )
	α LogSettingsClientMAwait::Suspend()ι->void{
		try{
			auto args = _mutation.ExtrapolateVariables();
			UpdateRuntime<RemoteLog>( args, "appServer" );
			Update( move(args) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}