#include <jde/web/server/SettingQL.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/web/server/IApplicationServer.h>
#include <jde/web/server/Sessions.h>
#define let const auto

namespace Jde::Web::Server{

	struct SettingQLAwait final : TAwait<jvalue>{
		SettingQLAwait( const QL::TableQL& query, SL sl )ι: TAwait<jvalue>{ sl }, _query{ query }{}

		α await_ready()ι->bool override{
			_result = _query.DefaultResult();
			try{
				let targets = Json::AsValue( _query.Args, "target" );
				Json::Visit( targets, [&]( const sv& target ){
					jobject setting;
					jvalue jv;
					if( target=="restSessionTimeout" )
						jv = Chrono::ToString<steady_clock::duration>( Sessions::RestSessionTimeout() );
					else if( target=="serverInstance" ){
						jv = IApplicationServer::InstancePK();
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
		QL::TableQL _query;
		up<exception> _exception;
		jvalue _result;
	};

	α SettingQL::Select( const QL::TableQL& query, UserPK /*executer*/, SL sl )ι->up<TAwait<jvalue>>{
		return query.JsonName.starts_with( "setting" ) ? mu<SettingQLAwait>( query, sl ) : nullptr;
	}
}