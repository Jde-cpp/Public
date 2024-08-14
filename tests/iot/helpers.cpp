#include "helpers.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "../../../Framework/source/db/Database.h"
#include <jde/iot/IotGraphQL.h>

#define var const auto

namespace Jde::Iot{
	constexpr ELogTags _tags{ ELogTags::Test };
	static Iot::IotGraphQL* _pHook;

	α CreateOpcServerAwait::Execute()ι->Jde::Task{
		var certificateUri{ "urn:open62541.server.application" };
		var create = 𐢜( "{{ mutation createOpcServer(  'input': {{'target':'{}','name':'My Test Server','certificateUri':'{}','description':'Test basic functionality','url':'opc.tcp://127.0.0.1:4840','isDefault':false}} ){{id}} }}", OpcServerTarget, certificateUri );
		var createJson = ( co_await DB::CoQuery( Str::Replace(create, '\'', '"'), 0, "CreateOpcServerAwait") ).UP<json>();
		Trace( _tags, "CreateOpcServer={}", createJson->dump() );
		Resume( Json::Getε<OpcPK>(*createJson, {"data", "opcServer", "id"}) );
	}

	α PurgeOpcServerAwait::Execute()ι->Jde::Task{
		if( !_pk.has_value() )
			_pk = SelectOpcServer()["id"].get<uint>();
		var q = 𐢜( "{{ mutation purgeOpcServer('id':{}) }}", *_pk );
		var result = ( co_await DB::CoQuery(Str::Replace(q, '\'', '"'), 0, "PurgeOpcServer") ).UP<json>();
		Trace( _tags, "PurgeOpcServer={}", result->dump() );
		ResumeScaler( 1 );
	}
}

namespace Jde{
	α Iot::CreateOpcServer()ι->OpcPK{
		atomic_flag done;
		OpcPK y;
		[&]()->CreateOpcServerAwait::Task {
			y = co_await CreateOpcServerAwait();
			done.test_and_set();
			done.notify_one();
		}();
		done.wait( false );
		return y;
	}

	α Iot::PurgeOpcServer( optional<OpcPK> pk )ι->uint{
		atomic_flag done;
		uint y;
		[=](uint& y, atomic_flag& done)->PurgeOpcServerAwait::Task {
			y = co_await PurgeOpcServerAwait( pk );
			done.test_and_set();
			done.notify_one();
		}( y, done );
		done.wait( false );
		return y;
	}

	α Iot::SelectOpcServer( uint id )ι->json{
		var subQuery = id ? 𐢜( "id:{{eq:{}}}", id ) : 𐢜( "target: {{eq:\"{}\"}}", OpcServerTarget );
		var select = 𐢜( "{{ query opcServer(filter:{{ {} }}){{ id name attributes created updated deleted target description certificateUri isDefault url }} }}", subQuery );
		var selectJson = DB::Query( select, 0 );
		Trace( _tags, "SelectOpcServer={}", selectJson.dump() );
		return selectJson["data"].is_null() ? json{} : selectJson["data"]["opcServer"];
	}

	α Iot::GetHook()ι->IotGraphQL*{ return _pHook; }
	α Iot::AddHook()ι->void{
		auto p = mu<IotGraphQL>();
		_pHook = p.get();
		DB::GraphQL::Hook::Add( move(p) );
	}
}