#include "helpers.h"
#include <jde/framework/settings.h>
#include <jde/opc/OpcQLHook.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include "../src/StartupAwait.h"

#define let const auto

namespace Jde::Opc{
	using Gateway::QL;
	constexpr ELogTags _tags{ ELogTags::Test };
	static Opc::OpcQLHook* _pHook;

	α CreateOpcClientAwait::Execute()ι->QL::QLAwait<jobject>::Task{
		try{
			let certificateUri{ Settings::FindSV("/opc/urn").value_or("urn:open62541.server.application") };
			let url{ Settings::FindSV("/opc/url").value_or( "opc.tcp://127.0.0.1:4840") };
			let create = Ƒ( "mutation createOpcClient( target:'{}', name:'My Test Server', certificateUri:'{}', description:'Test basic functionality', url:'{}', isDefault:false ){{id}}", OpcServerTarget, certificateUri, url );
			let createJson = co_await *QL().QueryObject( Str::Replace(create, '\'', '"'), {UserPK::System}, true, _sl );
			ResumeScaler( Json::AsNumber<OpcClientPK>(createJson, "id") );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α PurgeOpcClientAwait::Execute()ι->QL::QLAwait<>::Task{
		if( !_pk.has_value() )
			_pk = Json::AsNumber<OpcClientPK>( SelectOpcClient(OpcServerTarget), "id" );
		let q = Ƒ( "{{ mutation purgeOpcClient('id':{}) }}", *_pk );
		let result = co_await *QL().Query( Str::Replace(q, '\'', '"'), {UserPK::System}, true, _sl );
		ResumeScaler( 1 );
	}
}

namespace Jde{
	α Opc::CreateOpcClient()ι->OpcClientPK{
		atomic_flag done;
		OpcClientPK y;
		[&]()->CreateOpcClientAwait::Task {
			y = co_await CreateOpcClientAwait();
			done.test_and_set();
			done.notify_one();
		}();
		done.wait( false );
		return y;
	}

	α Opc::PurgeOpcClient( optional<OpcClientPK> pk )ι->uint{
		return BlockAwait<PurgeOpcClientAwait,uint>( PurgeOpcClientAwait{pk} );
	}

	α Opc::SelectOpcClient( DB::Key id )ι->jobject{
		let subQuery = id.IsPrimary() ? Ƒ( "client_id:{{eq:{}}}", id.PK() ) : Ƒ( "target: {{eq:\"{}\"}}", id.NK() );
		let select = Ƒ( "opcClient(filter:{{ {} }}){{ client_id name attributes created updated deleted target description certificateUri isDefault url }}", subQuery );
		return QL().QuerySync<>( select, {UserPK::System} );
	}

	α Opc::GetHook()ι->Opc::OpcQLHook*{ return _pHook; }
	α Opc::AddHook()ι->void{
		auto p = mu<Opc::OpcQLHook>();
		_pHook = p.get();
		QL::Hook::Add( move(p) );
	}
}