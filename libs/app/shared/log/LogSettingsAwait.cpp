#include <jde/app/log/LogSettingsAwait.h>
#include <jde/app/IApp.h>
#include <jde/app/log/ProtoLog.h>
#include <jde/app/client/RemoteLog.h>

#define let const auto

namespace Jde::App{
	α LogSettingsAwait::CalcResult()ι->jobject{
		jobject y;
		if( auto text = _ql.FindColumn("text"); text )
			y["text"] = ToJson<Logging::SpdLog>();
		if( auto binary = _ql.FindColumn("binary"); binary )
			y["binary"] = ToJson<App::ProtoLog>();
		// if( auto appServer = _ql.FindColumn("appServer"); appServer ){
		// 	auto logger = App::Client::RemoteLog::Instance();
		// 	y["appServer"] = logger ? ToJson( *logger ) : jobject{};
		// }
		if( auto tags = _ql.FindColumn("tags"); tags ){
			jobject jtags;
			for( let& [tag,value] : Logging::Tags(true) )
				jtags[tag] = value;
			y["tags"] = move( jtags );
		}
		return y;
	}
	//logSetting{ text binary appServer tags }
	α LogSettingsAwait::await_ready()ι->bool{
		_result = CalcResult();
		return true;
	}
	α LogSettingsAwait::await_resume()ε->jvalue{
		return move( _result );
	}

	α LogSettingsAwait::ToJson( const Logging::ILogger& logger )ι->jobject{
		jobject y;
		y["default"] = ToString( logger.DefaultLevel() );
		logger.ConfiguredTags().cvisit_all( [&](let& kv){
			y[Jde::ToString( kv.first, false )] = Jde::ToString( kv.second );
		});
		return y;
	}

	//updateLogSettings( text(default: Information,settings: Warning), binary(...) )
	α LogSettingsMAwait::Suspend()ι->void{
			Update( _mutation.ExtrapolateVariables() );
	}
	α LogSettingsMAwait::Update( jobject&& args )ι->void{
		try{
			UpdateRuntime<Logging::SpdLog>( args, "text" );
			UpdateRuntime<App::ProtoLog>( args, "binary" );
			//persist:false - the app server pushed levels it has already written to instance_tag_levels.  Writing them back
			//would re-enter updateInstanceTagLevel there, which would push again, and neither side would ever settle.
			if( let persist = args.if_contains("persist"); persist && persist->is_bool() && !persist->get_bool() ){
				Resume( jvalue{true} );
				return;
			}
			THROW_IF( !_appClient->InstancePK(), "No App InstancePK available for LogSettings update." );
			auto m = _mutation;
			m.Args.erase( "persist" );
			//updateLogSetting takes tag->level; updateInstanceTagLevel takes a record per override, since a combined tag is
			//no object key there.  Convert on the way out or the app server sees an object where it wants a list.
			for( let type : {"text", "binary", "appServer"} ){
				if( auto group = m.Args.if_contains(type); group && group->is_object() )
					*group = ToTagLevelArray( group->get_object() );
			}
			m.Args["id"] = _appClient->InstancePK();
			m.JsonTableName = "instanceTagLevels";
			m.CommandName = "updateInstanceTagLevel";
			UpdateApp( move(m) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α LogSettingsMAwait::UpdateApp( QL::MutationQL&& m )ι->TAwait<jvalue>::Task{
		try{
			Resume( co_await *_appClient->Query<jvalue>(m.ToString(), m.Variables ? *m.Variables : jobject{}) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}