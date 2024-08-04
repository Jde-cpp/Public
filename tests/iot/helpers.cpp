#include "helpers.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "../../../Framework/source/db/Database.h"
#include <jde/iot/IotGraphQL.h>

#define var const auto

namespace Jde{
	constexpr ELogTags _tags{ ELogTags::Test };
	static Iot::IotGraphQL* _pHook;

	α Iot::CreateOpcServer()ι->uint{
		var certificateUri{ "urn:open62541.server.application" };
		var create = 𐢜( "{{ mutation createOpcServer(  'input': {{'target':'{}','name':'My Test Server','certificateUri':'{}','description':'Test basic functionality','url':'opc.tcp://127.0.0.1:4840','isDefault':false}} ){{id}} }}", OpcServerTarget, certificateUri );
		var createJson = DB::Query( Str::Replace(create, '\'', '"'), 0 );
		Trace( _tags, "CreateOpcServer={}", createJson.dump() );
		return createJson["data"]["opcServer"]["id"].get<int>();
	}
	α Iot::SelectOpcServer( uint id )ι->json{
		var subQuery = id ? 𐢜( "id:{{eq:{}}}", id ) : 𐢜( "target: {{eq:\"{}\"}}", OpcServerTarget );
		var select = 𐢜( "{{ query opcServer(filter:{{ {} }}){{ id name attributes created updated deleted target description certificateUri isDefault url }} }}", subQuery );
		var selectJson = DB::Query( select, 0 );
		Trace( _tags, "SelectOpcServer={}", selectJson.dump() );
		return selectJson["data"].is_null() ? json{} : selectJson["data"]["opcServer"];
	}

	α Iot::PurgeOpcServer( uint pk )ι->void{
		if( pk==0 )
			pk = SelectOpcServer()["id"].get<uint>();
		var create = 𐢜( "{{ mutation purgeOpcServer('id':{}) }}", pk );
		var createJson = DB::Query( Str::Replace(create, '\'', '"'), 0 );
		Trace( _tags, "PurgeOpcServer={}", createJson.dump() );
	}

	α Iot::GetHook()ι->IotGraphQL*{ return _pHook; }
	α Iot::AddHook()ι->void{
		auto p = mu<IotGraphQL>();
		_pHook = p.get();
		DB::GraphQL::Hook::Add( move(p) );
	}
}