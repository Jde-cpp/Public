#include "InstanceTagLevelAwait.h"
#include <jde/db/IDataSource.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/app/usings.h>
#include "../LogData.h"


#define let const auto
namespace Jde::App::Server{

	//logSettings( programInstanceId:42 ){ text binary tags }
	α InstanceTagLevelAwait::Execute()ι->TAwait<vector<DB::Row>>::Task{
		try{
			auto schema = AppSchema();
			let instanceId = _mutation.Id();
			let& table = schema->GetView( "instance_tag_levels" );
			DB::Sql sql{
				Ƒ( "select type, tag, level_id from {} where instance_id=?", table.DBName, instanceId ),
				{ {instanceId} }
			};
			auto rows = co_await schema->DS()->SelectAsync( move(sql), _sl );

			auto text = _mutation.FindColumn("text") ? jobject{} : optional<jobject>{};
			auto binary = _mutation.FindColumn("binary") ? jobject{} : optional<jobject>{};
			auto appServer = _mutation.FindColumn("appServer") ? jobject{} : optional<jobject>{};
			for( auto&& row : rows ){
				let type = row.GetString( 0 );
				let tag = row.GetUInt( 1 );
				let tagName = tag==0 ? "default" : ToString( (ELogTags)tag );
				let level = (ELogLevel)row.GetUInt8Opt(2).value_or(0);
				if( type=="text" && text )
					(*text)[tagName] = ToString( level );
				else if( type=="binary" && binary )
					(*binary)[tagName] = ToString( level );
				else if( type=="appServer" && appServer )
					(*appServer)[tagName] = ToString( level );
			}
			jobject y;
			if( text )
				y["text"] = move(*text);
			if( binary )
				y["binary"] = move(*binary);
			if( appServer )
				y["appServer"] = move(*appServer);
			Resume( y );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	//updateLogSettings( programInstanceId: 42, text(default: Information,settings: Warning), binary(...) )
	α InstanceTagLevelMAwait::Update()ι->TAwait<uint32>::Task{
		try{
			auto vars = _mutation.ExtrapolateVariables();
			let instanceId = vars.at("id").to_number<ProgInstPK>();
			auto schema = AppSchema();
			let& table = schema->GetView( "instance_tag_levels" );
			auto sql = [&,instanceId]( str type, sv tag, jvalue level )->DB::Sql {
				DB::Value dbTag = tag=="default" ? DB::Value{0} : DB::Value{ underlying(ToLogTags(tag)) };
				return {
					Ƒ("{}(?,?,?,?)", table.UpsertProcName()),
					{ {instanceId}, {type}, dbTag, {underlying(ToLogLevel(level.as_string()))} },
					true
				};
			};
			uint rowCount{};
			if( let text = vars.if_contains("text"); text && text->is_object() ){
				for( auto&& [tag,level] : text->get_object() )
					rowCount += co_await schema->DS()->Execute( sql("text", tag, level) );
			}
			if( let binary = vars.if_contains("binary"); binary && binary->is_object() ){
				for( auto&& [tag,level] : binary->get_object() )
					rowCount += co_await schema->DS()->Execute( sql("binary", tag, level) );
			}
			if( let appServer = vars.if_contains("appServer"); appServer && appServer->is_object() ){
				for( auto&& [tag,level] : appServer->get_object() )
					rowCount += co_await schema->DS()->Execute( sql("appServer", tag, level) );
			}
			Resume( jvalue{rowCount} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}