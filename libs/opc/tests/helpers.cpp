#include "helpers.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "../../../Framework/source/db/Database.h"
#include <jde/opc/IotGraphQL.h>

#define let const auto

namespace Jde::Opc{
	constexpr ELogTags _tags{ ELogTags::Test };
	static Opc::IotGraphQL* _pHook;

	α CreateOpcServerAwait::Execute()ι->Jde::Task{
		let certificateUri{ Settings::Get("opc/urn").value_or("urn:open62541.server.application") };
		let url{ Settings::Get("opc/url").value_or( "opc.tcp://127.0.0.1:4840") };
		let create = Ƒ( "{{ mutation createOpcServer(  'input': {{'target':'{}','name':'My Test Server','certificateUri':'{}','description':'Test basic functionality','url':'{}','isDefault':false}} ){{id}} }}", OpcServerTarget, certificateUri, url );
		let createJson = ( co_await DB::CoQuery( Str::Replace(create, '\'', '"'), 0, "CreateOpcServerAwait") ).UP<json>();
		Trace( _tags, "CreateOpcServer={}", createJson->dump() );
		Resume( Json::Getε<OpcPK>(*createJson, {"data", "opcServer", "id"}) );
	}

	α PurgeOpcServerAwait::Execute()ι->Jde::Task{
		if( !_pk.has_value() )
			_pk = SelectOpcServer()["id"].get<uint>();
		let q = Ƒ( "{{ mutation purgeOpcServer('id':{}) }}", *_pk );
		let result = ( co_await DB::CoQuery(Str::Replace(q, '\'', '"'), 0, "PurgeOpcServer") ).UP<json>();
		Trace( _tags, "PurgeOpcServer={}", result->dump() );
		ResumeScaler( 1 );
	}
}

namespace Jde{
	α Opc::CreateOpcServer()ι->OpcPK{
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

	α Opc::PurgeOpcServer( optional<OpcPK> pk )ι->uint{
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

	α Opc::SelectOpcServer( uint id )ι->json{
		let subQuery = id ? Ƒ( "id:{{eq:{}}}", id ) : Ƒ( "target: {{eq:\"{}\"}}", OpcServerTarget );
		let select = Ƒ( "{{ query opcServer(filter:{{ {} }}){{ id name attributes created updated deleted target description certificateUri isDefault url }} }}", subQuery );
		let selectJson = DB::Query( select, 0 );
		Trace( _tags, "SelectOpcServer={}", selectJson.dump() );
		return Json::FindDefaultObject( selectJson, "data/opcServer" );
		selectJson["data"].is_null() ? json{} : selectJson["data"]["opcServer"];
	}

	α Opc::GetHook()ι->IotGraphQL*{ return _pHook; }
	α Opc::AddHook()ι->void{
		auto p = mu<IotGraphQL>();
		_pHook = p.get();
		DB::GraphQL::Hook::Add( move(p) );
	}
}