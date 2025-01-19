#include "helpers.h"
#include <jde/framework/settings.h>
#include <jde/opc/OpcQLHook.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/ql/ql.h>

#define let const auto

namespace Jde{
	//static sp<DB::AppSchema> _schema;
	//α Opc::SetTestSchema( sp<DB::AppSchema> opc )ι->void{ _schema = opc; }
	//α Opc::GetViewPtr( str name )ι->sp<DB::View>{ return _schema->GetViewPtr( name ); }
}
namespace Jde::Opc{
	constexpr ELogTags _tags{ ELogTags::Test };
	static Opc::OpcQLHook* _pHook;

	α CreateOpcServerAwait::Execute()ι->QL::QLAwait<>::Task{
		let certificateUri{ Settings::FindSV("opc/urn").value_or("urn:open62541.server.application") };
		let url{ Settings::FindSV("opc/url").value_or( "opc.tcp://127.0.0.1:4840") };
		let create = Ƒ( "mutation createOpcServer( target:'{}', name:'My Test Server', certificateUri:'{}', description:'Test basic functionality', url:'{}', isDefault:false ){{id}}", OpcServerTarget, certificateUri, url );
		let createJson = co_await QL::QLAwait( Str::Replace(create, '\'', '"'), {UserPK::System}, _sl );
		ResumeScaler( Json::AsNumber<OpcPK>(Json::AsObject(createJson), "createOpcServer/id") );
	}

	α PurgeOpcServerAwait::Execute()ι->QL::QLAwait<>::Task{
		if( !_pk.has_value() )
			_pk = Json::AsNumber<OpcPK>( SelectOpcServer(OpcServerTarget), "id" );
		let q = Ƒ( "{{ mutation purgeOpcServer('id':{}) }}", *_pk );
		let result = co_await QL::QLAwait( Str::Replace(q, '\'', '"'), {UserPK::System}, _sl );
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
		return BlockAwait<PurgeOpcServerAwait,uint>( PurgeOpcServerAwait{pk} );
	}

	α Opc::SelectOpcServer( DB::Key id )ι->jobject{
		let subQuery = id.IsPrimary() ? Ƒ( "server_id:{{eq:{}}}", id.PK() ) : Ƒ( "target: {{eq:\"{}\"}}", id.NK() );
		let select = Ƒ( "server(filter:{{ {} }}){{ server_id name attributes created updated deleted target description certificateUri isDefault url }}", subQuery );
		return QL::QueryObject( select, {UserPK::System} );
	}

	α Opc::GetHook()ι->Opc::OpcQLHook*{ return _pHook; }
	α Opc::AddHook()ι->void{
		auto p = mu<Opc::OpcQLHook>();
		_pHook = p.get();
		QL::Hook::Add( move(p) );
	}
}