#include "helpers.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "../../../Framework/source/db/Database.h"

#define var const auto

namespace Jde{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	α Iot::CreateOpcServer()ι->uint{
		var certificateUri{ "urn:open62541.server.application" };
		var create = Jde::format( "{{ mutation {{ createOpcServer(  'input': {{'target':'{}','name':'My Test Server','certificateUri':'{}','description':'Test basic functionality','url':'opc.tcp://127.0.0.1:4840','isDefault':false}} ){{id}} }} }}", OpcServerTarget, certificateUri );
		var createJson = DB::Query( Str::Replace(create, '\'', '"'), 0 );
		TRACE( "CreateOpcServer={}", createJson.dump() );
		return createJson["data"]["opcServer"]["id"].get<int>();
	}
	α Iot::SelectOpcServer( uint id )ι->json{
		var subQuery = id ? Jde::format( "id:{{eq:{}}}", id ) : Jde::format( "target: {{eq:\"{}\"}}", OpcServerTarget );
		var select = Jde::format( "query{{ opcServer(filter:{{ {} }}){{ id name attributes created updated deleted target description certificateUri isDefault url }} }}", subQuery );
		var selectJson = DB::Query( select, 0 );
		TRACE( "SelectOpcServer={}", selectJson.dump() );
		return selectJson["data"].is_null() ? json{} : selectJson["data"]["opcServer"];
	}

	α Iot::PurgeOpcServer( uint id )ι->void{
		var create = Jde::format( "{{ mutation {{ purgeOpcServer('id':{}) }} }}", id );
		var createJson = DB::Query( Str::Replace(create, '\'', '"'), 0 );
		TRACE( "PurgeOpcServer={}", createJson.dump() );
	}
}