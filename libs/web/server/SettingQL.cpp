#include <jde/web/server/SettingQL.h>
#include <jde/fwk/co/Await.h>
#include <jde/app/IApp.h>
#include <jde/web/server/Sessions.h>
#define let const auto

namespace Jde::Web::Server{

	struct SettingQLAwait final : TAwait<jvalue>{
		SettingQLAwait( const QL::TableQL& query, sp<App::IApp> appClient, SL sl )ι: TAwait<jvalue>{ sl }, _appClient{move(appClient)}, _query{ query }{}

		α await_ready()ι->bool override{
			_result = _query.DefaultResult();
			try{
				let targets = Json::AsValue( _query.Args, "target" );
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
		α Suspend()ι->void override{ ASSERT(false); }
		α await_resume()ε->jvalue override{
			if( _exception )
				throw *move( _exception );
			return _result;
		}
	private:
		sp<App::IApp> _appClient;
		up<exception> _exception;
		QL::TableQL _query;
		jvalue _result;
	};

	α SettingQL::Select( const QL::TableQL& query, UserPK /*executer*/, SL sl )ι->up<TAwait<jvalue>>{
		return query.JsonName.starts_with( "setting" ) ? mu<SettingQLAwait>( query, _appClient, sl ) : nullptr;
	}
}