#include <jde/app/log/LogSettingsAwait.h>
#include <jde/app/IApp.h>
#include <jde/app/log/ProtoLog.h>
#include <jde/app/client/RemoteLog.h>
#include <jde/fwk/str.h>
#include <jde/fwk/log/SpdLog.h>	//no longer reachable through <jde/fwk.h>

#define let const auto

namespace Jde::App{
	//L9: an unrecognised name was not rejected, it was dropped - ToLogTags warns and returns whatever it did recognise, which for a
	//name it recognises nothing of is ELogTags::None.  SetLevel then wrote a None row into _configuredTags that MinLevel can never
	//match, ToJson reported it back to the ui as a "none" tag, and the typo went on to updateInstanceTagLevel to be persisted as tag
	//0 - the row "default" uses.  Component-wise, because ToLogTags splits on TagSeparator and ORs the parts it knows: "socket.bogus"
	//resolved to a plain socket override, wider than what was asked for, with nothing to say so.
	α ValidateTagKeys( const jobject& args )ε->void{
		let catalogue = Logging::Tags( true );//the catalogue the logSetting query answers with - a name the ui was offered must not then be refused.
		for( let type : {"text", "binary", "appServer"} ){
			let group = args.if_contains( type );
			if( !group || !group->is_object() )
				continue;
			for( let& [key, _] : group->get_object() ){
				if( key==Logging::BreakTag || key=="default" )//neither is an ELogTags; UpdateRuntime handles both ahead of ToLogTags.
					continue;
				let name = string{ key };
				let parts = Str::Split( sv{name}, TagSeparator );
				THROW_IF( parts.empty(), "'{}' is not a log tag.", name );
				for( let& part : parts ){
					if( catalogue.contains(string{part}) )
						continue;
					THROW( "'{}' is not a log tag{}.", part, part==name ? string{} : Ƒ(" - in '{}'", name) );
				}
			}
		}
	}
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