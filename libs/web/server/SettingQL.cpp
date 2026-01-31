#include <jde/web/server/SettingQL.h>
#include <jde/fwk/co/Await.h>
#include <jde/app/IApp.h>
#include <jde/web/server/Sessions.h>
#define let const auto

namespace Jde::Web::Server{
	α SettingQLAwait::await_ready()ι->bool{
		_result = _query.DefaultResult();
		try{
			let targets = _query.As<jvalue>("target", _sl);
			Json::Visit( targets, [&]( const sv& target ){
				jobject setting;
				jvalue jv;
				if( target=="restSessionTimeout" )
					jv = Chrono::ToString<steady_clock::duration>( Sessions::RestSessionTimeout() );
				else if( target=="serverConnection" ){
					jv = _appClient->ConnectionPK();
				}else{
					let value = Settings::FindString( Ƒ("/http/clientSettings/{}", target) );
					jv = value ? jvalue{ *value } : jvalue{ nullptr };
				}
				if( _query.FindColumn("target") )
					setting["target"] = target;
				if( _query.FindColumn("value") )
					setting["value"] = jv;

				Json::AddOrAssign( _result, setting );
			});
		}
		catch( IException& e ){
			_exception = mu<Exception>( move(e) );
		}
		return true;
	}
	α SettingQLAwait::await_resume()ε->jvalue{
		if( _exception )
			throw *move( _exception );
		return _result;
	}
}